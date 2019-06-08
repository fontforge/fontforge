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
#include "ggadget.h"
#include "ggadgetP.h"

#include <math.h>

static void FigureBorderCols(GBox *design, Color *cols) {
    if ( design->border_type==bt_box || design->border_type==bt_double ) {
	cols[0] = design->border_brightest;
	cols[1] = design->border_brighter;
	cols[2] = design->border_darkest;
	cols[3] = design->border_darker;
    } else if ( design->border_type==bt_raised || design->border_type==bt_embossed ) {
	if ( design->flags&box_generate_colors ) {
	    int r = COLOR_RED(design->border_brightest), g=COLOR_GREEN(design->border_brightest), b = COLOR_BLUE(design->border_brightest);
	    cols[0] = design->border_brightest;
	    cols[1] = COLOR_CREATE(15*r/16,15*g/16,15*b/16);
	    cols[2] = COLOR_CREATE(7*r/16,7*g/16,7*b/16);
	    cols[3] = COLOR_CREATE(r/2,g/2,b/2);
	} else {
	    cols[0] = design->border_brightest;
	    cols[1] = design->border_brighter;
	    cols[2] = design->border_darkest;
	    cols[3] = design->border_darker;
	}
    } else if ( design->border_type==bt_lowered || design->border_type==bt_engraved ) {
	if ( design->flags&box_generate_colors ) {
	    int r = COLOR_RED(design->border_brightest), g=COLOR_GREEN(design->border_brightest), b = COLOR_BLUE(design->border_brightest);
	    cols[2] = design->border_brightest;
	    cols[3] = COLOR_CREATE(15*r/16,15*g/16,15*b/16);
	    cols[0] = COLOR_CREATE(7*r/16,7*g/16,7*b/16);
	    cols[1] = COLOR_CREATE(r/2,g/2,b/2);
	} else {
	    cols[2] = design->border_brightest;
	    cols[3] = design->border_brighter;
	    cols[0] = design->border_darkest;
	    cols[1] = design->border_darker;
	}
    }
}

static void DrawLeftTrap(GWindow gw,GRect *pos,int inset,int width,Color col) {
    GPoint pts[5];

    --width;
    if ( width==0 ) {
	GDrawDrawLine(gw,pos->x+inset,pos->y+inset,pos->x+inset,pos->y+pos->height-1-inset,col);
    } else {
	pts[0].x = pos->x+inset;
		pts[0].y = pos->y+inset;
	pts[1].x = pos->x+inset+width;
		pts[1].y = pos->y+inset+width;
	pts[2].x = pts[1].x;
		pts[2].y = pos->y+pos->height-1-(inset+width);
	pts[3].x = pts[0].x;
		pts[3].y = pos->y+pos->height-1-inset;
	pts[4] = pts[0];
	GDrawFillPoly(gw,pts,5,col);
    }
}

static void DrawULTrap(GWindow gw,GRect *pos,int inset,int width,Color col) {
    GPoint pts[5];
    /* for drawing a diamond shape */

    --width;
    if ( width==0 ) {
	GDrawDrawLine(gw,pos->x+inset,pos->y+pos->height/2,pos->x+pos->width/2,pos->y+inset,col);
    } else {
	pts[0].x = pos->x+inset;
		pts[0].y = pos->y+pos->height/2;
	pts[1].x = pos->x+inset+width;
		pts[1].y = pts[0].y;
	pts[2].x = pos->x+pos->width/2;
		pts[2].y = pos->y+inset+width;
	pts[3].x = pts[2].x;
		pts[3].y = pos->y+inset;
	pts[4] = pts[0];
	GDrawFillPoly(gw,pts,5,col);
    }
}

static void DrawTopTrap(GWindow gw,GRect *pos,int inset,int width,Color col) {
    GPoint pts[5];

    --width;
    if ( width==0 ) {
	GDrawDrawLine(gw,pos->x+inset,pos->y+inset,pos->x+pos->width-1-inset,pos->y+inset,col);
    } else {
	pts[0].x = pos->x+inset;
		pts[0].y = pos->y+inset;
	pts[1].x = pos->x+inset+width;
		pts[1].y = pos->y+inset+width;
	pts[2].x = pos->x+pos->width-1-(inset+width);
		pts[2].y = pts[1].y;
	pts[3].x = pos->x+pos->width-1-inset;
		pts[3].y = pts[0].y;
	pts[4] = pts[0];
	GDrawFillPoly(gw,pts,5,col);
    }
}

static void DrawURTrap(GWindow gw,GRect *pos,int inset,int width,Color col) {
    GPoint pts[5];

    --width;
    if ( width==0 ) {
	GDrawDrawLine(gw,pos->x+pos->width-1-inset,pos->y+pos->height/2,pos->x+pos->width/2,pos->y+inset,col);
    } else {
	pts[0].x = pos->x+pos->width-1-inset;
		pts[0].y = pos->y+pos->height/2;
	pts[1].x = pts[0].x-width;
		pts[1].y = pts[0].y;
	pts[2].x = pos->x+pos->width/2;
		pts[2].y = pos->y+inset+width;
	pts[3].x = pts[2].x;
		pts[3].y = pts[2].y-width;
	pts[4] = pts[0];
	GDrawFillPoly(gw,pts,5,col);
    }
}

static void DrawRightTrap(GWindow gw,GRect *pos,int inset,int width,Color col) {
    GPoint pts[5];

    --width;
    if ( width==0 ) {
	GDrawDrawLine(gw,pos->x+pos->width-1-inset,pos->y+inset,pos->x+pos->width-1-inset,pos->y+pos->height-1-inset,col);
    } else {
	pts[0].x = pos->x+pos->width-1-inset;
		pts[0].y = pos->y+inset;
	pts[1].x = pos->x+pos->width-1-(inset+width);
		pts[1].y = pos->y+inset+width;
	pts[2].x = pts[1].x;
		pts[2].y = pos->y+pos->height-1-(inset+width);
	pts[3].x = pts[0].x;
		pts[3].y = pos->y+pos->height-1-inset;
	pts[4] = pts[0];
	GDrawFillPoly(gw,pts,5,col);
    }
}

static void DrawLRTrap(GWindow gw,GRect *pos,int inset,int width,Color col) {
    GPoint pts[5];

    --width;
    if ( width==0 ) {
	GDrawDrawLine(gw,pos->x+pos->width-1-inset,pos->y+pos->height/2,pos->x+pos->width/2,pos->y+pos->height-1-inset,col);
    } else {
	pts[0].x = pos->x+pos->width-1-inset;
		pts[0].y = pos->y+pos->height/2;
	pts[1].x = pts[0].x-width;
		pts[1].y = pts[0].y;
	pts[2].x = pos->x+pos->width/2;
		pts[2].y = pos->y+pos->height-1-inset-width;
	pts[3].x = pts[2].x;
		pts[3].y = pts[2].y+width;
	pts[4] = pts[0];
	GDrawFillPoly(gw,pts,5,col);
    }
}

static void DrawBottomTrap(GWindow gw,GRect *pos,int inset,int width,Color col) {
    GPoint pts[5];

    --width;
    if ( width==0 ) {
	GDrawDrawLine(gw,pos->x+inset,pos->y+pos->height-1-inset,pos->x+pos->width-1-inset,pos->y+pos->height-1-inset,col);
    } else {
	pts[0].x = pos->x+inset;
		pts[0].y = pos->y+pos->height-1-inset;
	pts[1].x = pos->x+inset+width;
		pts[1].y = pos->y+pos->height-1-(inset+width);
	pts[2].x = pos->x+pos->width-1-(inset+width);
		pts[2].y = pts[1].y;
	pts[3].x = pos->x+pos->width-1-inset;
		pts[3].y = pts[0].y;
	pts[4] = pts[0];
	GDrawFillPoly(gw,pts,5,col);
    }
}

static void DrawLLTrap(GWindow gw,GRect *pos,int inset,int width,Color col) {
    GPoint pts[5];

    --width;
    if ( width==0 ) {
	GDrawDrawLine(gw,pos->x+inset,pos->y+pos->height/2,pos->x+pos->width/2,pos->y+pos->height-1-inset,col);
    } else {
	pts[0].x = pos->x+inset;
		pts[0].y = pos->y+pos->height/2;
	pts[1].x = pts[0].x+width;
		pts[1].y = pts[0].y;
	pts[2].x = pos->x+pos->width/2;
		pts[2].y = pos->y+pos->height-1-inset-width;
	pts[3].x = pts[2].x;
		pts[3].y = pts[2].y+width;
	pts[4] = pts[0];
	GDrawFillPoly(gw,pts,5,col);
    }
}

static void GetULRect(GRect *res, GRect *rect,int inset,int radius) {
    res->x = rect->x+inset; res->y = rect->y+inset;
    res->width = res->height = 2*(radius-inset);
}

static void DrawULArc(GWindow gw, GRect *rect,int inset,int radius, Color col) {
    GRect r;
    if ( inset>=radius )
return;
    GetULRect(&r,rect,inset,radius);
    GDrawDrawArc(gw,&r,90*64,90*64,col);
}

static void DrawULArcs(GWindow gw, GRect *rect,int inset,int radius, Color col1, Color col2) {
    GRect r;
    if ( inset>=radius )
return;
    GetULRect(&r,rect,inset,radius);
    if ( col1==col2 )
	GDrawDrawArc(gw,&r,90*64,90*64,col2);
    else {
	GDrawDrawArc(gw,&r,135*64,45*64,col1);
	GDrawDrawArc(gw,&r,90*64,45*64,col2);
    }
}

static void GetURRect(GRect *res, GRect *rect,int inset,int radius) {
    res->width = res->height = 2*(radius-inset);
    res->x = rect->x+rect->width-1-inset-res->width; res->y = rect->y+inset;
}

static void DrawURArc(GWindow gw, GRect *rect,int inset,int radius, Color col) {
    GRect r;
    if ( inset>=radius )
return;
    GetURRect(&r,rect,inset,radius);
    GDrawDrawArc(gw,&r,0*64,90*64,col);
}

static void DrawURArcs(GWindow gw, GRect *rect,int inset,int radius, Color col1, Color col2) {
    GRect r;
    if ( inset>=radius )
return;
    GetURRect(&r,rect,inset,radius);
    if ( col1==col2 )
	GDrawDrawArc(gw,&r,0*64,90*64,col2);
    else {
	GDrawDrawArc(gw,&r,45*64,45*64,col1);
	GDrawDrawArc(gw,&r,0*64,45*64,col2);
    }
}

static void GetLRRect(GRect *res, GRect *rect,int inset,int radius) {
    res->width = res->height = 2*(radius-inset);
    res->x = rect->x+rect->width-1-inset-res->width;
    res->y = rect->y+rect->height-1-inset-res->height;
}

static void DrawLRArc(GWindow gw, GRect *rect,int inset,int radius, Color col) {
    GRect r;
    if ( inset>=radius )
return;
    GetLRRect(&r,rect,inset,radius);
    GDrawDrawArc(gw,&r,-90*64,90*64,col);
}

static void DrawLRArcs(GWindow gw, GRect *rect,int inset,int radius, Color col1, Color col2) {
    GRect r;
    if ( inset>=radius )
return;
    GetLRRect(&r,rect,inset,radius);
    if ( col1==col2 )
	GDrawDrawArc(gw,&r,-90*64,90*64,col2);
    else {
	GDrawDrawArc(gw,&r,-45*64,45*64,col1);
	GDrawDrawArc(gw,&r,-90*64,45*64,col2);
    }
}

static void GetLLRect(GRect *res, GRect *rect,int inset,int radius) {
    res->width = res->height = 2*(radius-inset);
    res->x = rect->x+inset;
    res->y = rect->y+rect->height-1-inset-res->height;
}

static void DrawLLArc(GWindow gw, GRect *rect,int inset,int radius, Color col) {
    GRect r;
    if ( inset>=radius )
return;
    GetLLRect(&r,rect,inset,radius);
    GDrawDrawArc(gw,&r,-180*64,90*64,col);
}

static void DrawLLArcs(GWindow gw, GRect *rect,int inset,int radius, Color col1, Color col2) {
    GRect r;
    if ( inset>=radius )
return;
    GetLLRect(&r,rect,inset,radius);
    if ( col1==col2 )
	GDrawDrawArc(gw,&r,-180*64,90*64,col2);
    else {
	GDrawDrawArc(gw,&r,-135*64,45*64,col1);
	GDrawDrawArc(gw,&r,-180*64,45*64,col2);
    }
}

static void DrawRoundRect(GWindow gw, GRect *pos,int inset,int radius,
	Color col) {
    int off;
    if ( inset<radius ) {
	off = radius;
	DrawULArc(gw,pos,inset,radius,col);
	DrawURArc(gw,pos,inset,radius,col);
	DrawLRArc(gw,pos,inset,radius,col);
	DrawLLArc(gw,pos,inset,radius,col);
    } else
	off = inset;
    GDrawDrawLine(gw,pos->x+inset,pos->y+off,pos->x+inset,pos->y+pos->height-1-off,col);
    GDrawDrawLine(gw,pos->x+off,pos->y+inset,pos->x+pos->width-1-off,pos->y+inset,col);
    GDrawDrawLine(gw,pos->x+pos->width-1-inset,pos->y+off,pos->x+pos->width-1-inset,pos->y+pos->height-1-off,col);
    GDrawDrawLine(gw,pos->x+off,pos->y+pos->height-1-inset,pos->x+pos->width-1-off,pos->y+pos->height-1-inset,col);
}

static void DrawRoundTab(GWindow gw, GRect *pos,int inset,int radius,
	Color col1, Color col2, Color col3, Color col4, int active) {
    int off;
    if ( inset<radius ) {
	off = radius;
	DrawULArc(gw,pos,inset,radius,col1);
	DrawURArc(gw,pos,inset,radius,col3);
    } else
	off = inset;
    GDrawDrawLine(gw,pos->x+inset,pos->y+off,pos->x+inset,pos->y+pos->height-1,col1);
    GDrawDrawLine(gw,pos->x+off,pos->y+inset,pos->x+pos->width-1-off,pos->y+inset,col2);
    GDrawDrawLine(gw,pos->x+pos->width-1-inset,pos->y+off,pos->x+pos->width-1-inset,pos->y+pos->height-1,col3);
    if ( !active )
	GDrawDrawLine(gw,pos->x,pos->y+pos->height-1,pos->x+pos->width-1,pos->y+pos->height-1,col4);
}

static void DrawRoundRects(GWindow gw, GRect *pos,int inset,int radius,
	Color col1, Color col2, Color col3, Color col4) {
    int off;
    if ( inset<radius ) {
	off = radius;
	DrawULArcs(gw,pos,inset,radius,col1,col2);
	DrawURArcs(gw,pos,inset,radius,col2,col3);
	DrawLRArcs(gw,pos,inset,radius,col3,col4);
	DrawLLArcs(gw,pos,inset,radius,col4,col1);
    } else
	off = inset;
    GDrawDrawLine(gw,pos->x+inset,pos->y+off,pos->x+inset,pos->y+pos->height-1-off,col1);
    GDrawDrawLine(gw,pos->x+off,pos->y+inset,pos->x+pos->width-1-off,pos->y+inset,col2);
    GDrawDrawLine(gw,pos->x+pos->width-1-inset,pos->y+off,pos->x+pos->width-1-inset,pos->y+pos->height-1-off,col3);
    GDrawDrawLine(gw,pos->x+off,pos->y+pos->height-1-inset,pos->x+pos->width-1-off,pos->y+pos->height-1-inset,col4);
}

static int GBoxRectBorder(GWindow gw,GRect *pos,GBox *design,
	enum gadget_state state, int is_def) {
    int bw = GDrawPointsToPixels(gw,design->border_width);
    int inset = 0;
    GRect cur;
    int scale = GDrawPointsToPixels(gw,1);
    enum border_type bt = design->border_type;
    Color cols[4];
    Color fg = state==gs_disabled?design->disabled_foreground:
		    design->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(gw)):
		    design->main_foreground;
	Color color_inner = design->border_inner == COLOR_DEFAULT ? fg : design->border_inner;
	Color color_outer = design->border_outer == COLOR_DEFAULT ? fg : design->border_outer;

    FigureBorderCols(design,cols);
    if ( is_def && (design->flags & box_draw_default) && bt!=bt_none ) {
	DrawLeftTrap(gw,pos,inset,scale,cols[2]);
	DrawTopTrap(gw,pos,inset,scale,cols[3]);
	DrawRightTrap(gw,pos,inset,scale,cols[0]);
	DrawBottomTrap(gw,pos,inset,scale,cols[1]);
	inset = scale + GDrawPointsToPixels(gw,2);
    }

    if ( (design->flags & (box_foreground_border_outer|box_foreground_shadow_outer)) ) {
	GDrawSetLineWidth(gw,scale);
	cur = *pos;
	cur.x += inset; cur.y += inset; cur.width -= 2*inset; cur.height -= 2*inset;
	if ( scale>1 ) {
	    cur.x += scale/2; cur.y += scale/2;
	    cur.width -= scale; cur.height -= scale;
	}
	--cur.width; --cur.height;
	if ( design->flags&box_foreground_border_outer )
	    GDrawDrawRect(gw,&cur,color_outer);
	else {
	    GDrawDrawLine(gw,cur.x+scale,cur.y+cur.height,cur.x+cur.width,cur.y+cur.height,fg);
	    GDrawDrawLine(gw,cur.x+cur.width,cur.y+scale,cur.x+cur.width,cur.y+cur.height,fg);
	}
	inset += scale;
    }

    if ( bt==bt_double && bw<3 )
	bt = bt_box;
    if (( bt==bt_engraved || bt==bt_embossed ) && bw<2 )
	bt = bt_box;

    if ( bw!=0 ) switch ( bt ) {
      case bt_none:
      break;
      case bt_box: case bt_raised: case bt_lowered:
	DrawLeftTrap(gw,pos,inset,bw,cols[0]);
	DrawTopTrap(gw,pos,inset,bw,cols[1]);
	DrawRightTrap(gw,pos,inset,bw,cols[2]);
	DrawBottomTrap(gw,pos,inset,bw,cols[3]);
      break;
      case bt_engraved: case bt_embossed:
	bw &= ~1;
	DrawLeftTrap(gw,pos,inset,bw/2,cols[0]);
	DrawTopTrap(gw,pos,inset,bw/2,cols[1]);
	DrawRightTrap(gw,pos,inset,bw/2,cols[2]);
	DrawBottomTrap(gw,pos,inset,bw/2,cols[3]);

	inset += bw/2;
	DrawLeftTrap(gw,pos,inset,bw/2,cols[2]);
	DrawTopTrap(gw,pos,inset,bw/2,cols[3]);
	DrawRightTrap(gw,pos,inset,bw/2,cols[0]);
	DrawBottomTrap(gw,pos,inset,bw/2,cols[1]);
	inset -= bw/2;
      break;
      case bt_double: {
	int width = (bw+1)/3;
	DrawLeftTrap(gw,pos,inset,width,cols[0]);
	DrawTopTrap(gw,pos,inset,width,cols[1]);
	DrawRightTrap(gw,pos,inset,width,cols[2]);
	DrawBottomTrap(gw,pos,inset,width,cols[3]);

	inset += bw-width;
	DrawLeftTrap(gw,pos,inset,width,cols[0]);
	DrawTopTrap(gw,pos,inset,width,cols[1]);
	DrawRightTrap(gw,pos,inset,width,cols[2]);
	DrawBottomTrap(gw,pos,inset,width,cols[3]);
	inset -= bw-width;
      } break;
    }
    inset += bw;

    if ( (design->flags & box_foreground_border_inner) ||
	    ((design->flags & box_active_border_inner) && state==gs_active)) {
	GDrawSetLineWidth(gw,scale);
	cur = *pos;
	cur.x += inset; cur.y += inset;
	cur.width -= 2*inset; cur.height -= 2*inset;
	if ( scale>1 ) {
	    cur.x += scale/2; cur.y += scale/2;
	    cur.width -= scale; cur.height -= scale;
	}
	--cur.width; --cur.height;
	GDrawDrawRect(gw,&cur,
		state == gs_active && (design->flags & box_active_border_inner) ?
			design->active_border : color_inner);
	inset += scale;
    }
return( inset );
}

int GBoxDrawHLine(GWindow gw,GRect *pos,GBox *design) {
    int bw = GDrawPointsToPixels(gw,design->border_width);
    int x,y,xend;
    int scale = GDrawPointsToPixels(gw,1);
    enum border_type bt = design->border_type;
    Color cols[4];
    Color fg = design->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(gw)):
		    design->main_foreground;
	Color color_inner = design->border_inner == COLOR_DEFAULT ? fg : design->border_inner;
	Color color_outer = design->border_outer == COLOR_DEFAULT ? fg : design->border_outer;
    int bp = GBoxBorderWidth(gw,design);

    FigureBorderCols(design,cols);

    x = pos->x; xend = x+pos->width -1;
    y = pos->y + (pos->height-bp)/2;

    if ( (design->flags & box_foreground_border_outer) ) {
	GDrawSetLineWidth(gw,scale);
	GDrawDrawLine(gw,x,y+scale/2,xend,y+scale/2,color_outer);
	y += scale;
    }

    if ( bt==bt_double && bw<3 )
	bt = bt_box;
    if (( bt==bt_engraved || bt==bt_embossed ) && bw<2 )
	bt = bt_box;

    if ( bw!=0 ) switch ( bt ) {
      case bt_none:
      break;
      case bt_box: case bt_raised: case bt_lowered:
	GDrawSetLineWidth(gw,bw);
	GDrawDrawLine(gw,x,y+bw/2,xend,y+bw/2,cols[1]);
      break;
      case bt_engraved: case bt_embossed:
	bw &= ~1;
	GDrawSetLineWidth(gw,bw/2);
	GDrawDrawLine(gw,x,y+bw/4,xend,y+bw/4,cols[1]);
	y += bw/2;
	GDrawDrawLine(gw,x,y+bw/4,xend,y+bw/4,cols[3]);
	y -= bw/2;
      break;
      case bt_double: {
	int width = (bw+1)/3;
	GDrawSetLineWidth(gw,width);
	GDrawDrawLine(gw,x,y+width/2,xend,y+width/2,cols[1]);
	y += bw-width;
	GDrawDrawLine(gw,x,y+width/2,xend,y+width/2,cols[1]);
	y -= bw-width;
      } break;
    }
    y += bw;

    if ( design->flags & box_foreground_border_inner ) {
	GDrawSetLineWidth(gw,scale);
	GDrawDrawLine(gw,x,y+scale/2,xend,y+scale/2,color_inner);
	y += scale;
    }
return( y );
}

int GBoxDrawVLine(GWindow gw,GRect *pos,GBox *design) {
    int bw = GDrawPointsToPixels(gw,design->border_width);
    int x,y,yend;
    int scale = GDrawPointsToPixels(gw,1);
    enum border_type bt = design->border_type;
    Color cols[4];
    Color fg = design->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(gw)):
		    design->main_foreground;
	Color color_inner = design->border_inner == COLOR_DEFAULT ? fg : design->border_inner;
	Color color_outer = design->border_outer == COLOR_DEFAULT ? fg : design->border_outer;
    int bp = GBoxBorderWidth(gw,design);

    FigureBorderCols(design,cols);

    x = pos->x + (pos->width-bp)/2;
    y = pos->y; yend = y+pos->height -1;

    if ( (design->flags & box_foreground_border_outer) ) {
	GDrawSetLineWidth(gw,scale);
	GDrawDrawLine(gw,x+scale/2,y,x+scale/2,yend,color_outer);
	x += scale;
    }

    if ( bt==bt_double && bw<3 )
	bt = bt_box;
    if (( bt==bt_engraved || bt==bt_embossed ) && bw<2 )
	bt = bt_box;

    if ( bw!=0 ) switch ( bt ) {
      case bt_none:
      break;
      case bt_box: case bt_raised: case bt_lowered:
	GDrawSetLineWidth(gw,bw);
	GDrawDrawLine(gw,x+bw/2,y,x+bw/2,yend,cols[0]);
      break;
      case bt_engraved: case bt_embossed:
	bw &= ~1;
	GDrawSetLineWidth(gw,bw/2);
	GDrawDrawLine(gw,x+bw/4,y,x+bw/4,yend,cols[0]);
	x += bw/2;
	GDrawDrawLine(gw,x+bw/4,y,x+bw/4,yend,cols[2]);
	x -= bw/2;
      break;
      case bt_double: {
	int width = (bw+1)/3;
	GDrawSetLineWidth(gw,width);
	GDrawDrawLine(gw,x+width/2,y,x+width/2,yend,cols[0]);
	x += bw-width;
	GDrawDrawLine(gw,x+width/2,y,x+width/2,yend,cols[0]);
	x -= bw-width;
      } break;
    }
    x += bw;

    if ( design->flags & box_foreground_border_inner ) {
	GDrawSetLineWidth(gw,scale);
	GDrawDrawLine(gw,x+scale/2,y,x+scale/2,yend,color_inner);
	x += scale;
    }
return( x );
}

static int GBoxRoundRectBorder(GWindow gw,GRect *pos,GBox *design,
	enum gadget_state state, int is_def) {
    int bw = GDrawPointsToPixels(gw,design->border_width);
    int inset = 0;
    int scale = GDrawPointsToPixels(gw,1);
    enum border_type bt = design->border_type;
    Color cols[4];
    int rr = GDrawPointsToPixels(gw,design->rr_radius);
    Color fg = state==gs_disabled?design->disabled_foreground:
		    design->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(gw)):
		    design->main_foreground;
	Color color_inner = design->border_inner == COLOR_DEFAULT ? fg : design->border_inner;
	Color color_outer = design->border_outer == COLOR_DEFAULT ? fg : design->border_outer;

    if ( rr==0 )
	rr = pos->width/2;
    if ( is_def && (design->flags & box_draw_default) )
	rr += scale + GDrawPointsToPixels(gw,2);
    if ( rr>pos->width/2 )
	rr = pos->width/2;
    if ( rr>pos->height/2 )
	rr = pos->height/2;
    if ( !(scale&1)) --scale;
    if ( scale==0 ) scale = 1;

    FigureBorderCols(design,cols);
    if ( is_def && (design->flags & box_draw_default) && bt!=bt_none ) {
	GDrawSetLineWidth(gw,scale);
	DrawRoundRects(gw,pos,inset+scale/2,rr,cols[2],cols[3],cols[0],cols[1]);
	inset += scale + GDrawPointsToPixels(gw,2);
    }

    if ( design->flags & (box_foreground_border_outer|box_foreground_shadow_outer) ) {
	GDrawSetLineWidth(gw,scale);
	if ( design->flags&box_foreground_border_outer )
	    DrawRoundRect(gw,pos,scale/2,rr,color_outer);
	else {
	    GDrawDrawLine(gw,pos->x+scale+rr,pos->y+pos->height,pos->x+pos->width,pos->y+pos->height,fg);
	    GDrawDrawLine(gw,pos->x+pos->width,pos->y+scale+rr,pos->x+pos->width,pos->y+pos->height,fg);
	}
	inset += scale;
    }

    if ( bt==bt_double && bw<3 )
	bt = bt_box;
    if (( bt==bt_engraved || bt==bt_embossed ) && bw<2 )
	bt = bt_box;

    if ( bw!=0 ) switch ( bt ) {
      case bt_none:
      break;
      case bt_box: case bt_raised: case bt_lowered:
	if ( !(bw&1) ) --bw;
	GDrawSetLineWidth(gw,bw);
	DrawRoundRects(gw,pos,inset+bw/2,rr,cols[0],cols[1],cols[2],cols[3]);
      break;
      case bt_engraved: case bt_embossed:
	bw &= ~1;
	if ( !(bw&2 ) )
	    bw -= 2;
	if ( bw<=0 )
	    bw = 2;
	GDrawSetLineWidth(gw,bw/2);
	DrawRoundRects(gw,pos,inset+bw/4,rr,cols[0],cols[1],cols[2],cols[3]);
	DrawRoundRects(gw,pos,inset+bw/2+bw/4,rr,cols[2],cols[3],cols[0],cols[1]);
      break;
      case bt_double: {
	int width = (bw+1)/3;
	if ( !(width&1) ) {
	    if ( 2*(width+1) < bw )
		++width;
	    else
		--width;
	}
	GDrawSetLineWidth(gw,width);
	DrawRoundRects(gw,pos,inset+width/2,rr,cols[0],cols[1],cols[2],cols[3]);
	DrawRoundRects(gw,pos,inset+bw-(width+1)/2,rr,cols[0],cols[1],cols[2],cols[3]);
      } break;
    }
    inset += bw;

    if ( (design->flags & box_foreground_border_inner) ||
	    ((design->flags & box_active_border_inner) && state==gs_active)) {
	GDrawSetLineWidth(gw,scale);
	DrawRoundRect(gw,pos,inset+scale/2,rr,
		state == gs_active && (design->flags & box_active_border_inner) ?
			design->active_border : color_inner);
	inset += scale;
    }
return( inset );
}

static int GBoxElipseBorder(GWindow gw,GRect *pos,GBox *design,
	enum gadget_state state, int is_def) {
    int bw = GDrawPointsToPixels(gw,design->border_width);
    int inset = 0;
    GRect cur;
    int scale = GDrawPointsToPixels(gw,1);
    enum border_type bt = design->border_type;
    Color cols[4];
    Color fg = state==gs_disabled?design->disabled_foreground:
		    design->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(gw)):
		    design->main_foreground;
	Color color_inner = design->border_inner == COLOR_DEFAULT ? fg : design->border_inner;
	Color color_outer = design->border_outer == COLOR_DEFAULT ? fg : design->border_outer;

    if ( !(scale&1)) --scale;
    if ( scale==0 ) scale = 1;

    FigureBorderCols(design,cols);
    if ( is_def && (design->flags & box_draw_default) && bt!=bt_none ) {
	int temp = scale;
	if ( !(temp&1) ) --temp;
	GDrawSetLineWidth(gw,temp);
	cur = *pos;
	cur.x += inset+temp/2; cur.y += inset+temp/2;
	cur.width -= 2*(inset+temp/2)+1; cur.height -= 2*(inset+temp/2)+1;
	GDrawDrawArc(gw,&cur,90*64,90*64,cols[2]);
	GDrawDrawArc(gw,&cur,0*64,90*64,cols[3]);
	GDrawDrawArc(gw,&cur,-90*64,90*64,cols[0]);
	GDrawDrawArc(gw,&cur,-180*64,90*64,cols[1]);
	inset += scale + GDrawPointsToPixels(gw,2);
    }

    if ( design->flags & box_foreground_border_outer ) {
	GDrawSetLineWidth(gw,scale);
	cur = *pos;
	if ( scale>1 ) {
	    cur.x += scale/2; cur.y += scale/2;
	    cur.width -= scale; cur.height -= scale;
	}
	--cur.width; --cur.height;
	GDrawDrawElipse(gw,&cur,color_outer);
	inset += scale;
    }

    if ( bt==bt_double && bw<3 )
	bt = bt_box;
    if (( bt==bt_engraved || bt==bt_embossed ) && bw<2 )
	bt = bt_box;
    FigureBorderCols(design,cols);

    if ( bw!=0 ) switch ( bt ) {
      case bt_none:
      break;
      case bt_box: case bt_raised: case bt_lowered:
	if ( !(bw&1) ) --bw;
	GDrawSetLineWidth(gw,bw);
	cur = *pos;
	cur.x += inset+bw/2; cur.y += inset+bw/2;
	cur.width -= 2*(inset+bw/2)+1; cur.height -= 2*(inset+bw/2)+1;
	GDrawDrawArc(gw,&cur,90*64,90*64,cols[0]);
	GDrawDrawArc(gw,&cur,0*64,90*64,cols[1]);
	GDrawDrawArc(gw,&cur,-90*64,90*64,cols[2]);
	GDrawDrawArc(gw,&cur,-180*64,90*64,cols[3]);
      break;
      case bt_engraved: case bt_embossed:
	bw &= ~1;
	if ( !(bw&2 ) )
	    bw -= 2;
	if ( bw<=0 )
	    bw = 2;
	GDrawSetLineWidth(gw,bw/2);
	cur = *pos;
	cur.x += inset+bw/4; cur.y += inset+bw/4;
	cur.width -= 2*(inset+bw/4)+1; cur.height -= 2*(inset+bw/4)+1;
	GDrawDrawArc(gw,&cur,90*64,90*64,cols[0]);
	GDrawDrawArc(gw,&cur,0*64,90*64,cols[1]);
	GDrawDrawArc(gw,&cur,-90*64,90*64,cols[2]);
	GDrawDrawArc(gw,&cur,-180*64,90*64,cols[3]);
	cur.x += bw/2; cur.y += bw/2;
	cur.width -= bw; cur.height -= bw;
	GDrawDrawArc(gw,&cur,90*64,90*64,cols[2]);
	GDrawDrawArc(gw,&cur,0*64,90*64,cols[3]);
	GDrawDrawArc(gw,&cur,-90*64,90*64,cols[0]);
	GDrawDrawArc(gw,&cur,-180*64,90*64,cols[1]);
	GDrawSetLineWidth(gw,scale);
      break;
      case bt_double: {
	int width = (bw+1)/3;
	if ( !(width&1) ) {
	    if ( 2*(width+1) < bw )
		++width;
	    else
		--width;
	}
	GDrawSetLineWidth(gw,width);
	cur = *pos;
	cur.x += inset+width/2; cur.y += inset+width/2;
	cur.width -= 2*(inset+width/2)+1; cur.height -= 2*(inset+width/2)+1;
	GDrawDrawArc(gw,&cur,90*64,90*64,cols[0]);
	GDrawDrawArc(gw,&cur,0*64,90*64,cols[1]);
	GDrawDrawArc(gw,&cur,-90*64,90*64,cols[2]);
	GDrawDrawArc(gw,&cur,-180*64,90*64,cols[3]);
	cur = *pos;
	cur.x += inset+bw-(width+1)/2; cur.y += inset+bw-(width+1)/2;
	cur.width -= 2*(inset+bw-(width+1)/2)+1; cur.height -= 2*(inset+bw-(width+1)/2)+1;
	GDrawDrawArc(gw,&cur,90*64,90*64,cols[0]);
	GDrawDrawArc(gw,&cur,0*64,90*64,cols[1]);
	GDrawDrawArc(gw,&cur,-90*64,90*64,cols[2]);
	GDrawDrawArc(gw,&cur,-180*64,90*64,cols[3]);
	GDrawSetLineWidth(gw,scale);
      } break;
    }
    inset += bw;

    if ( (design->flags & box_foreground_border_inner) ||
	    ((design->flags & box_active_border_inner) && state==gs_active)) {
	GDrawSetLineWidth(gw,scale);
	cur = *pos;
	cur.x += inset; cur.y += inset;
	cur.width -= 2*inset; cur.height -= 2*inset;
	if ( scale>1 ) {
	    cur.x += scale/2; cur.y += scale/2;
	    cur.width -= scale; cur.height -= scale;
	}
	--cur.width; --cur.height;
	GDrawDrawElipse(gw,&cur,
		state == gs_active && (design->flags & box_active_border_inner) ?
			design->active_border : color_inner);
	inset += scale;
    }
return( inset );
}

static int GBoxDiamondBorder(GWindow gw,GRect *pos,GBox *design,
	enum gadget_state state, int is_def) {
    int bw = GDrawPointsToPixels(gw,design->border_width);
    int inset = 0;
    int scale = GDrawPointsToPixels(gw,1);
    enum border_type bt = design->border_type;
    Color cols[4];
    Color fg = state==gs_disabled?design->disabled_foreground:
		    design->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(gw)):
		    design->main_foreground;
	Color color_inner = design->border_inner == COLOR_DEFAULT ? fg : design->border_inner;
	Color color_outer = design->border_outer == COLOR_DEFAULT ? fg : design->border_outer;

    FigureBorderCols(design,cols);
    if ( is_def && (design->flags & box_draw_default) && bt!=bt_none ) {
	DrawULTrap(gw,pos,inset,scale,cols[2]);
	DrawURTrap(gw,pos,inset,scale,cols[3]);
	DrawLRTrap(gw,pos,inset,scale,cols[0]);
	DrawLLTrap(gw,pos,inset,scale,cols[1]);
	inset += scale + GDrawPointsToPixels(gw,2);
    }

    if ( design->flags & box_foreground_border_outer ) {
	GPoint pts[5];
	GDrawSetLineWidth(gw,scale);
	pts[0].x = pos->x+scale/2; pts[0].y = pos->y+pos->height/2;
	pts[1].x = pos->x+pos->width/2; pts[1].y = pos->y+scale/2;
	pts[2].x = pos->x+pos->width-1-scale/2; pts[2].y = pts[0].y;
	pts[3].x = pts[1].x; pts[3].y = pos->y+pos->height-1-scale/2;
	pts[4] = pts[0];
	GDrawDrawPoly(gw,pts,5,color_outer);
	inset += scale;
    }

    if ( bt==bt_double && bw<3 )
	bt = bt_box;
    if (( bt==bt_engraved || bt==bt_embossed ) && bw<2 )
	bt = bt_box;
    FigureBorderCols(design,cols);

    if ( bw!=0 ) switch ( bt ) {
      case bt_none:
      break;
      case bt_box: case bt_raised: case bt_lowered:
	DrawULTrap(gw,pos,inset,bw,cols[0]);
	DrawURTrap(gw,pos,inset,bw,cols[1]);
	DrawLRTrap(gw,pos,inset,bw,cols[2]);
	DrawLLTrap(gw,pos,inset,bw,cols[3]);
      break;
      case bt_engraved: case bt_embossed:
	bw &= ~1;
	DrawULTrap(gw,pos,inset,bw/2,cols[0]);
	DrawURTrap(gw,pos,inset,bw/2,cols[1]);
	DrawLRTrap(gw,pos,inset,bw/2,cols[2]);
	DrawLLTrap(gw,pos,inset,bw/2,cols[3]);

	inset += bw/2;
	DrawULTrap(gw,pos,inset,bw/2,cols[2]);
	DrawURTrap(gw,pos,inset,bw/2,cols[3]);
	DrawLRTrap(gw,pos,inset,bw/2,cols[0]);
	DrawLLTrap(gw,pos,inset,bw/2,cols[1]);
	inset -= bw/2;
      break;
      case bt_double: {
	int width = (bw+1)/3;
	DrawULTrap(gw,pos,inset,width,cols[0]);
	DrawURTrap(gw,pos,inset,width,cols[1]);
	DrawLRTrap(gw,pos,inset,width,cols[2]);
	DrawLLTrap(gw,pos,inset,width,cols[3]);

	inset += bw-width;
	DrawULTrap(gw,pos,inset,width,cols[0]);
	DrawURTrap(gw,pos,inset,width,cols[1]);
	DrawLRTrap(gw,pos,inset,width,cols[2]);
	DrawLLTrap(gw,pos,inset,width,cols[3]);
	inset -= bw-width;
      } break;
    }
    inset += bw;

    if ( (design->flags & box_foreground_border_inner) ||
	    ((design->flags & box_active_border_inner) && state==gs_active)) {
	GPoint pts[5];
	GDrawSetLineWidth(gw,scale);
	pts[0].x = pos->x+inset+scale/2; pts[0].y = pos->y+pos->height/2;
	pts[1].x = pos->x+pos->width/2; pts[1].y = pos->y+inset+scale/2;
	pts[2].x = pos->x+pos->width-1-inset-scale/2; pts[2].y = pts[0].y;
	pts[3].x = pts[1].x; pts[3].y = pos->y+pos->height-1-inset-scale/2;
	pts[4] = pts[0];
	GDrawDrawPoly(gw,pts,5,
		state == gs_active && (design->flags & box_active_border_inner) ?
			design->active_border : color_inner);
	inset += scale;
    }
return( inset );
}

int GBoxDrawBorder(GWindow gw,GRect *pos,GBox *design,enum gadget_state state,
	int is_default) {
    int ret = 0;

    if ( state == gs_disabled )
	GDrawSetStippled(gw,1,0,0);
    switch ( design->border_shape ) {
      case bs_rect:
	ret = GBoxRectBorder(gw,pos,design,state,is_default);
      break;
      case bs_roundrect:
	ret = GBoxRoundRectBorder(gw,pos,design,state,is_default);
      break;
      case bs_elipse:
	ret = GBoxElipseBorder(gw,pos,design,state,is_default);
      break;
      case bs_diamond:
	ret = GBoxDiamondBorder(gw,pos,design,state,is_default);
      break;
    }
    if ( state == gs_disabled )
	GDrawSetStippled(gw,0,0,0);
return( ret );
}

static void BoxGradientRect(GWindow gw, GRect *r, Color start, Color end)
{
    int xend = r->x + r->width - 1;
    int i;

    int rstart = COLOR_RED(start);
    int gstart = COLOR_GREEN(start);
    int bstart = COLOR_BLUE(start);
    int rdiff = COLOR_RED(end) - rstart;
    int gdiff = COLOR_GREEN(end) - gstart;
    int bdiff = COLOR_BLUE(end) - bstart;

    if (r->height <= 0)
return;

    for (i = 0; i < r->height; i++)
	GDrawDrawLine(gw, r->x, r->y + i, xend, r->y + i, COLOR_CREATE(
		rstart + rdiff * i / r->height,
		gstart + gdiff * i / r->height,
		bstart + bdiff * i / r->height ));
}

static void BoxGradientElipse(GWindow gw, GRect *r, Color start, Color end)
{
    /*
     * Ellipse borders are 1 unit wider and 1 unit higher than the passed GRect.
     * With corrected values the gradient-fill will fit its corresponding border.
     */
    int correctedwidth = r->width + 1;
    int correctedheight = r->height + 1;

    int xend = r->x + correctedwidth - 1;
    int yend = r->y + correctedheight - 1;
    int i, xoff;

    double a = (double)correctedwidth / 2.0;
    double b = (double)correctedheight / 2.0;
    double precalc = (a * a) / (b * b);

    int rstart = COLOR_RED(start);
    int gstart = COLOR_GREEN(start);
    int bstart = COLOR_BLUE(start);
    int rdiff = COLOR_RED(end) - rstart;
    int gdiff = COLOR_GREEN(end) - gstart;
    int bdiff = COLOR_BLUE(end) - bstart;

    if (correctedheight <= 0)
return;

    for (i = 0; i < (correctedheight + 1) / 2; ++i) {
	xoff = lrint(a - sqrt(precalc * (double)(i * (correctedheight - i)) ));

	GDrawDrawLine(gw, r->x + xoff, r->y + i, xend - xoff, r->y + i, COLOR_CREATE(
		rstart + rdiff * i / correctedheight,
		gstart + gdiff * i / correctedheight,
		bstart + bdiff * i / correctedheight ));

	GDrawDrawLine(gw, r->x + xoff, yend - i, xend - xoff, yend - i, COLOR_CREATE(
		rstart + rdiff * (correctedheight - i) / correctedheight,
		gstart + gdiff * (correctedheight - i) / correctedheight,
		bstart + bdiff * (correctedheight - i) / correctedheight ));
    }
}

static void BoxGradientRoundRect(GWindow gw, GRect *r, int rr, Color start, Color end)
{
    int radius = rr <= (r->height+1)/2 ? (rr > 0 ? rr : 0) : (r->height+1)/2;
    int xend = r->x + r->width - 1;
    int yend = r->y + r->height - 1;
    int precalc = radius * 2 - 1;
    int i, xoff;

    int rstart = COLOR_RED(start);
    int gstart = COLOR_GREEN(start);
    int bstart = COLOR_BLUE(start);
    int rdiff = COLOR_RED(end) - rstart;
    int gdiff = COLOR_GREEN(end) - gstart;
    int bdiff = COLOR_BLUE(end) - bstart;

    if (r->height <= 0)
return;

    for (i = 0; i < radius; i++) {
	xoff = radius - lrint(sqrt( (double)(i * (precalc - i)) ));

	GDrawDrawLine(gw, r->x + xoff, r->y + i, xend - xoff, r->y + i, COLOR_CREATE(
		rstart + rdiff * i / r->height,
		gstart + gdiff * i / r->height,
		bstart + bdiff * i / r->height ));

	GDrawDrawLine(gw, r->x + xoff, yend - i, xend - xoff, yend - i, COLOR_CREATE(
		rstart + rdiff * (r->height - i) / r->height,
		gstart + gdiff * (r->height - i) / r->height,
		bstart + bdiff * (r->height - i) / r->height ));
    }

    for (i = radius; i < r->height - radius; i++)
	GDrawDrawLine(gw, r->x, r->y + i, xend, r->y + i, COLOR_CREATE(
		rstart + rdiff * i / r->height,
		gstart + gdiff * i / r->height,
		bstart + bdiff * i / r->height ));
}

void GBoxDrawBackground(GWindow gw,GRect *pos,GBox *design,
	enum gadget_state state, int is_default) {
    Color gbg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(gw));
    Color mbg = design->main_background==COLOR_DEFAULT?gbg:design->main_background;
    Color dbg = design->disabled_background==COLOR_DEFAULT?gbg:design->disabled_background;
    Color pbg = design->depressed_background==COLOR_DEFAULT?gbg:design->depressed_background;
    Color ibg;
    int def_off = is_default && (design->flags & box_draw_default) ?
	    GDrawPointsToPixels(gw,1) + GDrawPointsToPixels(gw,2): 0;

    if ( state == gs_disabled ) {
	ibg=dbg;
	GDrawSetStippled(gw,1,0,0);
    } else if ( state == gs_pressedactive && (design->flags & box_do_depressed_background ))
	ibg=pbg;
    else
	ibg=mbg;

    if ( design->border_shape==bs_rect && (def_off==0 || mbg==ibg) && !(design->flags & box_gradient_bg)) {
	GDrawFillRect(gw,pos,ibg);
    } else {
	/* GDrawFillRect(gw,pos,mbg); */
	if ( design->border_shape==bs_rect ) {
	    GRect cur;
	    cur = *pos;
	    cur.x += def_off; cur.y += def_off; cur.width -= 2*def_off; cur.height -= 2*def_off;
	    if ( design->flags & box_gradient_bg )
		BoxGradientRect(gw,&cur,ibg,design->gradient_bg_end);
	    else
		GDrawFillRect(gw,&cur,ibg);
	} else if ( design->border_shape==bs_elipse ) {
	    GRect cur;
	    cur = *pos; --cur.width; --cur.height;
	    if ( def_off ) {
		cur.x += def_off; cur.y += def_off; cur.width -= 2*def_off; cur.height -= 2*def_off;
	    }
	    if ( design->flags & box_gradient_bg )
		BoxGradientElipse(gw,&cur,ibg,design->gradient_bg_end);
	    else
		GDrawFillElipse(gw,&cur,ibg);
	} else if ( design->border_shape==bs_diamond ) {
	    GPoint pts[5];
	    pts[0].x = pos->x+pos->width/2;	    pts[0].y = pos->y+def_off;
	    pts[1].x = pos->x+pos->width-1-def_off; pts[1].y = pos->y+pos->height/2;
	    pts[2].x = pts[0].x;		    pts[2].y = pos->y+pos->height-1-def_off;
	    pts[3].x = pos->x+def_off;		    pts[3].y = pts[1].y;
	    pts[4] = pts[0];
	    GDrawFillPoly(gw,pts,5,ibg);
	} else {
	    int rr = GDrawPointsToPixels(gw,design->rr_radius);

	    if ( rr==0 )
		rr = pos->width/2-def_off;
	    if ( rr>pos->width/2-def_off )
		rr = pos->width/2-def_off;
	    if ( rr>pos->height/2-def_off )
		rr = pos->height/2-def_off;

	    if ( design->flags & box_gradient_bg )
		BoxGradientRoundRect(gw,pos,rr,ibg,design->gradient_bg_end);
	    else
		GDrawFillRoundRect(gw,pos,rr,ibg);
	}
    }
    if ( state == gs_disabled )
	GDrawSetStippled(gw,0,0,0);
}

int GBoxBorderWidth(GWindow gw, GBox *box) {
    int scale = GDrawPointsToPixels(gw,1);
    int bp = GDrawPointsToPixels(gw,box->border_width)+
	     GDrawPointsToPixels(gw,box->padding)+
	     ((box->flags & (box_foreground_border_outer|box_foreground_shadow_outer))?scale:0)+
	     ((box->flags &
		 (box_foreground_border_inner|box_active_border_inner))?scale:0);
return( bp );
}

int GBoxExtraSpace(GGadget *g) {
    if ( g->state==gs_invisible || !(g->box->flags & box_draw_default) ||
	    !GGadgetIsDefault(g))
return( 0 );

return( GDrawPointsToPixels(g->base,1) + GDrawPointsToPixels(g->base,2) );
}

/* Does not include the padding */
int GBoxDrawnWidth(GWindow gw, GBox *box) {
    int scale = GDrawPointsToPixels(gw,1);
    int bp = GDrawPointsToPixels(gw,box->border_width)+
	     ((box->flags & (box_foreground_border_outer|box_foreground_shadow_outer))?scale:0)+
	     ((box->flags &
		 (box_foreground_border_inner|box_active_border_inner))?scale:0);
return( bp );
}

static void GBoxDrawTabBackground(GWindow pixmap, GRect *rect, int radius, Color color)
{
    GRect older, r = *rect;

    GDrawPushClip(pixmap,rect,&older);
    GDrawSetLineWidth(pixmap,1);
    r.height*=2;
    if (2*radius>=r.height) r.height=2*radius+1;
    GDrawFillRoundRect(pixmap, &r, radius, color);
    GDrawPopClip(pixmap,&older);
}

void GBoxDrawTabOutline(GWindow pixmap, GGadget *g, int x, int y,
	int width, int rowh, int active ) {
    GRect r;
    GBox *design = g->box;
    int bp = GBoxBorderWidth(pixmap,design);
    int dw = GBoxDrawnWidth(pixmap,design);
    int rr = GDrawPointsToPixels(pixmap,design->rr_radius);
    int scale = GDrawPointsToPixels(pixmap,1);
    int bw = GDrawPointsToPixels(pixmap,design->border_width);
    int inset = 0;
    enum border_type bt = design->border_type;
    Color cols[4];

    Color fg = g->state==gs_disabled?design->disabled_foreground:
		    design->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
		    design->main_foreground;

    Color color_inner = design->border_inner == COLOR_DEFAULT ? fg : design->border_inner;
    Color color_outer = design->border_outer == COLOR_DEFAULT ? fg : design->border_outer;

    Color gbg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
    Color mbg = design->main_background==COLOR_DEFAULT?gbg:design->main_background;
    Color dbg = design->disabled_background==COLOR_DEFAULT?gbg:design->disabled_background;
    Color pbg = design->depressed_background==COLOR_DEFAULT?gbg:design->depressed_background;
    Color ibg;

    r.x = x; r.y = y; r.width = width; r.height = rowh;

    if ( rr==0 )
        rr = GDrawPointsToPixels(pixmap,3);

    if ( !(scale&1)) --scale;
    if ( scale==0 ) scale = 1;

    FigureBorderCols(design,cols);

    if ( active ) {
        r.x -= bp; r.y -= bp; r.width += 2*bp; r.height += dw+bp;
    }

    if ( g->state == gs_disabled ) {
        ibg=dbg;
        GDrawSetStippled(pixmap,1,0,0);
    } else if ( !active && (design->flags & box_do_depressed_background ))
        ibg=pbg;
    else
        ibg=mbg;

    GBoxDrawTabBackground(pixmap,&r,rr,ibg);
    if ( g->state == gs_disabled )
        GDrawSetStippled(pixmap,0,0,0);

    if ( design->flags & (box_foreground_border_outer|box_foreground_shadow_outer) ) {
	GDrawSetLineWidth(pixmap,scale);
	if ( design->flags&box_foreground_border_outer )
	    DrawRoundTab(pixmap,&r,scale/2,rr,color_outer,color_outer,color_outer,color_outer,active);
	else
	    GDrawDrawLine(pixmap,r.x+r.width-1,r.y+rr,r.x+r.width-1,r.y+r.height-1,fg);
	inset += scale;
    }

    if ( bt==bt_double && bw<3 )
	bt = bt_box;
    if (( bt==bt_engraved || bt==bt_embossed ) && bw<2 )
	bt = bt_box;

    if ( bw!=0 ) switch ( bt ) {
      case bt_none:
      break;
      case bt_box: case bt_raised: case bt_lowered:
	if ( !(bw&1) ) --bw;
	GDrawSetLineWidth(pixmap,bw);
	DrawRoundTab(pixmap,&r,inset+bw/2,rr,cols[0],cols[1],cols[2],cols[3],active);
      break;
      case bt_engraved: case bt_embossed:
	bw &= ~1;
	if ( !(bw&2 ) )
	    bw -= 2;
	if ( bw<=0 )
	    bw = 2;
	GDrawSetLineWidth(pixmap,bw/2);
	DrawRoundTab(pixmap,&r,inset+bw/4,rr,cols[0],cols[1],cols[2],cols[3],active);
	DrawRoundTab(pixmap,&r,inset+bw/2+bw/4,rr,cols[2],cols[3],cols[0],cols[1],active);
      break;
      case bt_double: {
	int width = (bw+1)/3;
	if ( !(width&1) ) {
	    if ( 2*(width+1) < bw )
		++width;
	    else
		--width;
	}
	GDrawSetLineWidth(pixmap,width);
	DrawRoundTab(pixmap,&r,inset+width/2,rr,cols[0],cols[1],cols[2],cols[3],active);
	DrawRoundTab(pixmap,&r,inset+bw-(width+1)/2,rr,cols[0],cols[1],cols[2],cols[3],active);
      } break;
    }
    inset += bw;

    if ( (design->flags & box_foreground_border_inner) ) {
	GDrawSetLineWidth(pixmap,scale);
	DrawRoundTab(pixmap,&r,inset+scale/2,rr,color_inner,color_inner,color_inner,color_inner,active);
	inset += scale;
    }
}
