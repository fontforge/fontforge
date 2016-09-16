/* -*- coding: utf-8 -*- */
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
#include "fontforgeui.h"
#include "cvundoes.h"
#include <gkeysym.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <gresource.h>
#include <gresedit.h>

#include "gutils/unicodelibinfo.h"

int bv_width = 270, bv_height=250;

extern int _GScrollBar_Width;
extern struct lconv localeinfo;
extern char *coord_sep;
struct bvshows BVShows = { 1, 1, 1, 0 };

#define RPT_BASE	3		/* Place to draw the pointer icon */
#define RPT_DATA	24		/* x,y text after above */
#define RPT_COLOR	40		/* Blob showing the foreground color */

static void BVNewScale(BitmapView *bv) {
    int fh = bv->bdf->ascent+bv->bdf->descent;

    GScrollBarSetBounds(bv->vsb,-2*fh*bv->scale,4*fh*bv->scale,bv->height);
    GScrollBarSetBounds(bv->hsb,-3*fh*bv->scale,6*fh*bv->scale,bv->width);
    GScrollBarSetPos(bv->vsb,bv->yoff);
    GScrollBarSetPos(bv->hsb,bv->xoff);

    GDrawRequestExpose(bv->v,NULL,false);
}

static void BVFit(BitmapView *bv) {
    int left, right, top, bottom, hsc, wsc;
    int fh = bv->bdf->ascent+bv->bdf->descent;
    extern int palettes_docked;
    int offset = palettes_docked ? 80 : 0;

    if ( offset>bv->width ) offset = 0;

    bottom = bv->bc->ymin;
    top = bv->bc->ymax;
    left = bv->bc->xmin;
    right = bv->bc->xmax;

    if ( bottom>0 ) bottom = 0;
    if ( left>0 ) left = 0;
    if ( top==-1 && bottom==0 ) {	/* Empty */
	top = bv->bdf->ascent;
	bottom = -bv->bdf->descent;
	if ( right==-1 ) right = fh;
    }
    if ( top<bottom ) IError("Bottom bigger than top!");
    if ( right<left ) IError("Left bigger than right!");
    top -= bottom;
    right -= left;
    if ( top==0 ) top = bv->bdf->pixelsize;
    if ( right==0 ) right = bv->bdf->pixelsize;
    wsc = (8*(bv->width-offset)) / (10*right);
    hsc = (8*bv->height) / (10*top);
    if ( wsc<hsc ) hsc = wsc;
    if ( hsc<=0 ) hsc = 1;
    if ( hsc>32 ) hsc = 32;

    bv->scale = hsc;

    bv->xoff = left+(bv->width-offset-right*bv->scale)/2 + offset;
    bv->yoff = bottom + (bv->height-top*bv->scale)/2;
    if ( bv->xoff<-3*fh*bv->scale ) bv->xoff = -3*fh*bv->scale;
    if ( bv->yoff<-2*fh*bv->scale ) bv->yoff = -2*fh*bv->scale;

    BVNewScale(bv);
}

static void BVUnlinkView(BitmapView *bv ) {
    BitmapView *test;

    if ( bv->bc->views == bv ) {
	bv->bc->views = bv->next;
    } else {
	for ( test=bv->bc->views; test->next!=bv && test->next!=NULL; test=test->next );
	if ( test->next==bv )
	    test->next = bv->next;
    }
    if ( bv->bc->views==NULL ) {
	/* We just got rid of the last view. Do a little clean up */
	/*  compress the bitmap, and get rid of the floating selection */
	BCCompressBitmap(bv->bc);
	BCFlattenFloat(bv->bc);
    }
}

static void BVRefreshImage(BitmapView *bv) {
    GRect box;

    box.x = 0; box.width = bv->infoh;
    box.y = bv->mbh; box.height = bv->infoh;
    GDrawRequestExpose(bv->gw,&box,false);
}

static void BCCharUpdate(BDFChar *bc) {
    BitmapView *bv;

    for ( bv = bc->views; bv!=NULL; bv=bv->next ) {
	GDrawRequestExpose(bv->v, NULL, false );
	/*BVRefreshImage(bv);*/		/* Select All gives us a blank image if we do this */
    }
}

static void BC_CharChangedUpdate(BDFChar *bc) {
    BDFFont *bdf;
    BitmapView *bv;
    int waschanged = bc->changed;
    FontView *fv;
    struct bdfcharlist *dlist;

    bc->changed = true;
    for ( bv = bc->views; bv!=NULL; bv=bv->next ) {
	GDrawRequestExpose(bv->v, NULL, false );
	BVRefreshImage(bv);
    }

    fv = (FontView *) (bc->sc->parent->fv);
    fv->b.sf->changed = true;
    if ( fv->show!=fv->filled ) {
	for ( bdf=fv->b.sf->bitmaps; bdf!=NULL && bdf->glyphs[bc->orig_pos]!=bc; bdf=bdf->next );
	if ( bdf!=NULL ) {
	    FVRefreshChar(fv,bc->orig_pos);
	    if ( fv->b.sf->onlybitmaps && !waschanged )
		FVToggleCharChanged(fv->b.sf->glyphs[bc->orig_pos]);
	}
    }
    for ( dlist=bc->dependents; dlist!=NULL; dlist=dlist->next )
	BC_CharChangedUpdate(dlist->bc);
}

static char *BVMakeTitles(BitmapView *bv, BDFChar *bc,char *buf) {
    char *title;
    SplineChar *sc;
    BDFFont *bdf = bv->bdf;
    char *uniname;

    sc = bc->sc;
/* GT: This is the title for a window showing a bitmap character */
/* GT: It will look something like: */
/* GT:  exclam at 33 size 12 from Arial */
/* GT: $1 is the name of the glyph */
/* GT: $2 is the glyph's encoding */
/* GT: $3 is the pixel size of the bitmap font */
/* GT: $4 is the font name */
    sprintf(buf,_("%1$.80s at %2$d size %3$d from %4$.80s"),
	    sc!=NULL ? sc->name : "<Nameless>", bv->enc, bdf->pixelsize, sc==NULL ? "" : sc->parent->fontname);
    title = copy(buf);

    /* Enhance 'buf' description with Nameslist.txt unicode name definition */
    if ( (uniname=unicode_name(sc->unicodeenc))!=NULL ) {
	strcat(buf, " ");
	strcpy(buf+strlen(buf), uniname);
	free(uniname);
    }
    return( title );
}

void BVChangeBC(BitmapView *bv, BDFChar *bc, int fitit ) {
    char *title;
    char buf[300];

    BVUnlinkView(bv);
    bv->bc = bc;
    bv->next = bc->views;
    bc->views = bv;

    if ( fitit )
	BVFit(bv);
    else
	BVNewScale(bv);
    BVRefreshImage(bv);

    title = BVMakeTitles(bv,bc,buf);
    GDrawSetWindowTitles8(bv->gw,buf,title);
    free(title);

    BVPaletteChangedChar(bv);
}

static void BVChangeChar(BitmapView *bv, int i, int fitit ) {
    BDFChar *bc;
    BDFFont *bdf = bv->bdf;
    EncMap *map = bv->fv->b.map;

    if ( bv->fv->b.cidmaster!=NULL && !map->enc->is_compact && i<bdf->glyphcnt &&
	    (bc=bdf->glyphs[i])!=NULL ) {
	/* The attached bitmap fonts don't have the complexities of subfonts-- they are flat */
    } else {
	if ( i<0 || i>=map->enccount )
return;
	bc = BDFMakeChar(bdf,map,i);
    }

    if ( bc==NULL || bv->bc == bc )
return;
    bv->map_of_enc = map;
    bv->enc = i;

    BVChangeBC(bv,bc,fitit);
}

static void BVDoClear(BitmapView *bv);
static void BVHScroll(BitmapView *bv,struct sbevent *sb);
static void BVVScroll(BitmapView *bv,struct sbevent *sb);

void BVChar(BitmapView *bv, GEvent *event ) {
    BDFRefChar *head;
    extern int navigation_mask;

#if _ModKeysAutoRepeat
	/* Under cygwin these keys auto repeat, they don't under normal X */
	if ( bv->autorpt!=NULL ) {
	    GDrawCancelTimer(bv->autorpt); bv->autorpt = NULL;
	    if ( bv->keysym == event->u.chr.keysym )	/* It's an autorepeat, ignore it */
return;
	    BVToolsSetCursor(bv,bv->oldstate,NULL);
	}
#endif

    BVPaletteActivate(bv);
    BVToolsSetCursor(bv,TrueCharState(event),NULL);
    if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuSaveAll(NULL,NULL,NULL);
    else if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->u.chr.keysym == GK_BackSpace ) {
	/* Menu does delete */
	BVDoClear(bv);
    } else if ( event->u.chr.keysym == GK_Help ) {
	MenuHelp(NULL,NULL,NULL);	/* Menu does F1 */
    } else if ( event->u.chr.keysym == GK_Left ||
	    event->u.chr.keysym == GK_Up ||
	    event->u.chr.keysym == GK_Right ||
	    event->u.chr.keysym == GK_Down ||
	    event->u.chr.keysym == GK_KP_Left ||
	    event->u.chr.keysym == GK_KP_Up ||
	    event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_KP_Down ||
	    event->u.chr.keysym == GK_KP_Home ||
	    event->u.chr.keysym == GK_Home ) {
	int xoff=0, yoff=0;
	if ( event->u.chr.keysym == GK_Up || event->u.chr.keysym == GK_KP_Up )
	    yoff = 1;
	else if ( event->u.chr.keysym == GK_Down || event->u.chr.keysym == GK_KP_Down )
	    yoff = -1;
	else if ( event->u.chr.keysym == GK_Right || event->u.chr.keysym == GK_KP_Right )
	    xoff = 1;
	else if ( event->u.chr.keysym == GK_Left || event->u.chr.keysym == GK_KP_Left )
	    xoff = -1;
	else if ( event->u.chr.keysym == GK_Home || event->u.chr.keysym == GK_KP_Home ) {
	    if ( bv->bc->selection==NULL ) {
		xoff = -bv->bc->xmin;
		yoff = -bv->bc->ymin;
	    } else {
		xoff = bv->bc->xmin-bv->bc->selection->xmin;
		yoff = bv->bc->ymin-bv->bc->selection->ymin;
	    }
	}
	if ( event->u.chr.state & (ksm_meta|ksm_control) ) {
	    struct sbevent sb;
	    sb.type = yoff>0 || xoff<0 ? et_sb_halfup : et_sb_halfdown;
	    if ( xoff==0 )
		BVVScroll(bv,&sb);
	    else
		BVHScroll(bv,&sb);
	} else {
	    BCPreserveState(bv->bc);
	    if ( bv->bc->selection==NULL ) {
		bv->bc->xmin += xoff;  bv->bc->xmax += xoff;
		bv->bc->ymin += yoff;  bv->bc->ymax += yoff;
		for ( head=bv->bc->refs; head!=NULL; head=head->next ) {
		    if ( head->selected ) {
			head->xoff += xoff;
			head->yoff += yoff;
		    }
		}
	    } else {
		bv->bc->selection->xmin += xoff;
		bv->bc->selection->xmax += xoff;
		bv->bc->selection->ymin += yoff;
		bv->bc->selection->ymax += yoff;
	    }
	    BCCharChangedUpdate(bv->bc);
	}
    } else if ( (event->u.chr.state&((GMenuMask()|navigation_mask)&~(ksm_shift|ksm_capslock)))==navigation_mask &&
	    event->type == et_char &&
	    event->u.chr.keysym!=0 &&
	    (event->u.chr.keysym<GK_Special /*|| event->u.chr.keysym>=0x10000*/)) {
	SplineFont *sf = bv->bc->sc->parent;
	int i;
	EncMap *map = bv->fv->b.map;
	extern int cv_auto_goto;
	if ( cv_auto_goto ) {
	    i = SFFindSlot(sf,map,event->u.chr.keysym,NULL);
	    if ( i!=-1 )
		BVChangeChar(bv,i,false);
	}
    }
}

static int BVCurEnc(BitmapView *bv) {
    if ( bv->map_of_enc == bv->fv->b.map && bv->enc!=-1 )
return( bv->enc );

return( bv->fv->b.map->backmap[bv->bc->orig_pos] );
}

static void BVCharUp(BitmapView *bv, GEvent *event ) {
    if ( event->u.chr.keysym=='I' &&
	    (event->u.chr.state&ksm_shift) &&
	    (event->u.chr.state&ksm_meta) )
	SCCharInfo(bv->bc->sc,bv->fv->b.active_layer,bv->fv->b.map,BVCurEnc(bv));
#if _ModKeysAutoRepeat
    /* Under cygwin these keys auto repeat, they don't under normal X */
    else if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ||
	    event->u.chr.keysym == GK_Control_L || event->u.chr.keysym == GK_Control_R ||
	    event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R ||
	    event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R ||
	    event->u.chr.keysym == GK_Super_L || event->u.chr.keysym == GK_Super_R ||
	    event->u.chr.keysym == GK_Hyper_L || event->u.chr.keysym == GK_Hyper_R ) {
	if ( bv->autorpt!=NULL ) {
	    GDrawCancelTimer(bv->autorpt);
	    BVToolsSetCursor(bv,bv->oldstate,NULL);
	}
	bv->keysym = event->u.chr.keysym;
	bv->oldstate = TrueCharState(event);
	bv->autorpt = GDrawRequestTimer(bv->v,100,0,NULL);
    } else {
	if ( bv->autorpt!=NULL ) {
	    GDrawCancelTimer(bv->autorpt); bv->autorpt=NULL;
	    BVToolsSetCursor(bv,bv->oldstate,NULL);
	}
	BVToolsSetCursor(bv,TrueCharState(event),NULL);
    }
#else
    BVToolsSetCursor(bv,TrueCharState(event),NULL);
#endif
}

static void BVDrawTempPoint(BitmapView *bv,int x, int y,void *pixmap) {
    GRect pixel;

    pixel.width = pixel.height = bv->scale+1;
    pixel.x = bv->xoff + x*bv->scale;
    pixel.y = bv->height-bv->yoff-(y+1)*bv->scale;
    GDrawSetStippled(pixmap,1, 0,0);
    GDrawFillRect(pixmap,&pixel,0x909000);
    GDrawSetStippled(pixmap,0, 0,0);
}

static void BVDrawRefBorder(BitmapView *bv, BDFChar *bc, GWindow pixmap,
	uint8 selected, int8 xoff, int8 yoff) {
    int i, j;
    int isblack, lw, rw, tw, bw;
    int tx, ty;
    Color outcolor = selected ? 0x606000 : 0x606060;

    for ( i=bc->ymax-bc->ymin; i>=0; --i ) {
	for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
	    tx = bv->xoff + (bc->xmin + xoff +j)*bv->scale;
	    ty = bv->height-bv->yoff - (bc->ymax + yoff - i)*bv->scale;

	    isblack = ( !bc->byte_data &&
		( bc->bitmap[i*bc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))))) ||
		( bc->byte_data && bc->bitmap[i*bc->bytes_per_line+j] != 0 );
	    if ( !isblack )
	continue;
	    lw = ( j == 0 || ( !bc->byte_data &&
		!( bc->bitmap[i*bc->bytes_per_line+((j-1)>>3)] & (1<<(7-((j-1)&7))))) ||
		( bc->byte_data && bc->bitmap[i*bc->bytes_per_line+j-1] == 0 ));
	    rw = ( j == bc->xmax-bc->xmin || ( !bc->byte_data &&
		!( bc->bitmap[i*bc->bytes_per_line+((j+1)>>3)] & (1<<(7-((j+1)&7))))) ||
		( bc->byte_data && bc->bitmap[i*bc->bytes_per_line+j+1] == 0 ));
	    tw = ( i == bc->ymax-bc->ymin || ( !bc->byte_data &&
		!( bc->bitmap[(i+1)*bc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))))) ||
		( bc->byte_data && bc->bitmap[(i+1)*bc->bytes_per_line+j] == 0 ));
	    bw = ( i == 0 || ( !bc->byte_data &&
		!( bc->bitmap[(i-1)*bc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))))) ||
		( bc->byte_data && bc->bitmap[(i-1)*bc->bytes_per_line+j] == 0 ));

	    if ( lw )
		GDrawDrawLine(pixmap,tx+1,ty, tx+1,ty-bv->scale,outcolor);
	    if ( rw )
		GDrawDrawLine(pixmap,tx+bv->scale-1,ty, tx+bv->scale-1,ty-bv->scale,outcolor);
	    if ( tw )
		GDrawDrawLine(pixmap,tx,ty-1, tx+bv->scale,ty-1,outcolor);
	    if ( bw )
		GDrawDrawLine(pixmap,tx,ty-bv->scale+1, tx+bv->scale,ty-bv->scale+1,outcolor);
	}
    }
}

static void BVDrawSelection(BitmapView *bv,void *pixmap) {
    GRect pixel, rect;
    BDFFloat *sel = bv->bc->selection;
    GClut *clut = bv->bdf->clut;
    Color bg = view_bgcol;
    int i,j;

    pixel.width = pixel.height = bv->scale+1;
    for ( i=sel->ymax-sel->ymin; i>=0; --i ) {
	for ( j=0; j<=sel->xmax-sel->xmin; ++j ) {
	    pixel.x = bv->xoff + (sel->xmin+j)*bv->scale;
	    pixel.y = bv->height-bv->yoff-(sel->ymax-i+1)*bv->scale;
	    if ( clut==NULL ) {
		if ( sel->bitmap[i*sel->bytes_per_line+(j>>3)] & (1<<(7-(j&7))) ) {
		    GDrawFillRect(pixmap,&pixel,0x808080);
		} else {
		    GDrawFillRect(pixmap,&pixel,bg);
		}
	    } else
		GDrawFillRect(pixmap,&pixel,
			clut->clut[sel->bitmap[i*sel->bytes_per_line+j]]);
	}
    }
    GDrawSetStippled(pixmap,1, 0,0);
    rect.width = (sel->xmax-sel->xmin+1)*bv->scale;
    rect.height = (sel->ymax-sel->ymin+1)*bv->scale;
    rect.x = bv->xoff + sel->xmin*bv->scale;
    rect.y = bv->height-bv->yoff-(sel->ymax+1)*bv->scale;
    GDrawFillRect(pixmap,&rect,0x909000);
    GDrawSetStippled(pixmap,0, 0,0);
}

static void BCBresenhamLine(BitmapView *bv,
	void (*SetPoint)(BitmapView *,int x, int y, void *data),void *data) {
    /* Draw a line from (pressed_x,pressed_y) to (info_x,info_y) */
    /*  and call SetPoint for each point */
    int dx,dy,incr1,incr2,d,x,y,xend;
    int x1 = bv->pressed_x, y1 = bv->pressed_y;
    int x2 = bv->info_x, y2 = bv->info_y;
    int up;

    if ( y2<y1 ) {
	y2 ^= y1; y1 ^= y2; y2 ^= y1;
	x2 ^= x1; x1 ^= x2; x2 ^= x1;
    }
    dy = y2-y1;
    if (( dx = x2-x1)<0 ) dx=-dx;

    if ( dy<=dx ) {
	d = 2*dy-dx;
	incr1 = 2*dy;
	incr2 = 2*(dy-dx);
	if ( x1>x2 ) {
	    x = x2; y = y2;
	    xend = x1;
	    up = -1;
	} else {
	    x = x1; y = y1;
	    xend = x2;
	    up = 1;
	}
	(SetPoint)(bv,x,y,data);
	while ( x<xend ) {
	    ++x;
	    if ( d<0 ) d+=incr1;
	    else {
		y += up;
		d += incr2;
	    }
	    (SetPoint)(bv,x,y,data);
	}
    } else {
	d = 2*dx-dy;
	incr1 = 2*dx;
	incr2 = 2*(dx-dy);
	x = x1; y = y1;
	if ( x1>x2 ) up = -1; else up = 1;
	(SetPoint)(bv,x,y,data);
	while ( y<y2 ) {
	    ++y;
	    if ( d<0 ) d+=incr1;
	    else {
		x += up;
		d += incr2;
	    }
	    (SetPoint)(bv,x,y,data);
	}
    }
}

static void CirclePoints(BitmapView *bv,int x, int y, int ox, int oy, int xmod, int ymod,
	void (*SetPoint)(BitmapView *,int x, int y, void *data),void *data) {
    /* we draw the quadrant between Pi/2 and 0 */
    if ( bv->active_tool == bvt_filledelipse ) {
	int j;
	for ( j=2*oy+ymod-y; j<=y; ++j ) {
	    SetPoint(bv,x,j,data);
	    SetPoint(bv,2*ox+xmod-x,j,data);
	}
    } else {
	SetPoint(bv,x,y,data);
	SetPoint(bv,x,2*oy+ymod-y,data);
	SetPoint(bv,2*ox+xmod-x,y,data);
	SetPoint(bv,2*ox+xmod-x,2*oy+ymod-y,data);
    }
}

void BCGeneralFunction(BitmapView *bv,
	void (*SetPoint)(BitmapView *,int x, int y, void *data),void *data) {
    int i, j;
    int xmin, xmax, ymin, ymax;
    int ox, oy, modx, mody;
    int dx, dy, c,d,dx2,dy2,xp,yp;
    int x,y;

    if ( bv->pressed_x<bv->info_x ) {
	xmin = bv->pressed_x; xmax = bv->info_x;
    } else {
	xmin = bv->info_x; xmax = bv->pressed_x;
    }
    if ( bv->pressed_y<bv->info_y ) {
	ymin = bv->pressed_y; ymax = bv->info_y;
    } else {
	ymin = bv->info_y; ymax = bv->pressed_y;
    }

    switch ( bv->active_tool ) {
      case bvt_line:
	BCBresenhamLine(bv,SetPoint,data);
      break;
      case bvt_rect:
	for ( i=xmin; i<=xmax; ++i ) {
	    SetPoint(bv,i,bv->pressed_y,data);
	    SetPoint(bv,i,bv->info_y,data);
	}
	for ( i=ymin; i<=ymax; ++i ) {
	    SetPoint(bv,bv->pressed_x,i,data);
	    SetPoint(bv,bv->info_x,i,data);
	}
      break;
      case bvt_filledrect:
	for ( i=xmin; i<=xmax; ++i ) {
	    for ( j=ymin; j<=ymax; ++j )
		SetPoint(bv,i,j,data);
	}
      break;
      case bvt_elipse: case bvt_filledelipse:
	if ( xmax==xmin || ymax==ymin )		/* degenerate case */
	    BCBresenhamLine(bv,SetPoint,data);
	else {
	    ox = floor( (xmin+xmax)/2.0 );
	    oy = floor( (ymin+ymax)/2.0 );
	    modx = (xmax+xmin)&1; mody = (ymax+ymin)&1;
	    dx = ox-xmin;
	    dy = oy-ymin;
	    dx2 = dx*dx; dy2 = dy*dy;
	    xp = 0; yp = 4*dy*dx2;
	    c = dy2+(2-4*dy)*dx2; d = 2*dy2 + (1-2*dy)*dx2;
	    x = ox+modx; y = ymax;
	    CirclePoints(bv,x,y,ox,oy,modx,mody,SetPoint,data);
	    while ( x!=xmax ) {
#define move_right() (c += 4*dy2+xp, d += 6*dy2+xp, ++x, xp += 4*dy2 )
#define move_down() (c += 6*dx2-yp, d += 4*dx2-yp, --y, yp -= 4*dx2 )
		if ( d<0 || y==0 )
		    move_right();
		else if ( c > 0 )
		    move_down();
		else {
		    move_right();
		    move_down();
		}
#undef move_right
#undef move_down
		if ( y<oy )		/* degenerate cases */
	    break;
		CirclePoints(bv,x,y,ox,oy,modx,mody,SetPoint,data);
	    }
	    if ( bv->active_tool==bvt_elipse ) {
		/* there may be quite a gap between the two semi-circles */
		/*  because the tangent is nearly vertical here. So just fill */
		/*  it in */
		int j;
		for ( j=2*oy+mody-y; j<=y; ++j ) {
		    SetPoint(bv,x,j,data);
		    SetPoint(bv,2*ox+modx-x,j,data);
		}
	    }
	}
      break;
    }
}

static void BVDrawRefName(BitmapView *bv,GWindow pixmap,BDFRefChar *ref,int fg) {
    int x,y, len;
    GRect size;
    char *refinfo;
    IBounds bb;

    refinfo = malloc(strlen(ref->bdfc->sc->name) +  30);
    sprintf(refinfo,"%s XOff: %d YOff: %d", ref->bdfc->sc->name, ref->xoff, ref->yoff);

    bb.minx = ref->bdfc->xmin + ref->xoff;
    bb.maxx = ref->bdfc->xmax + ref->xoff;
    bb.miny = ref->bdfc->ymin + ref->yoff;
    bb.maxy = ref->bdfc->ymax + ref->yoff;
    BDFCharQuickBounds(ref->bdfc,&bb,ref->xoff,ref->yoff,false,true);
    x = bv->xoff + (bb.minx)*bv->scale;
    y = bv->height - bv->yoff - (bb.maxy + 1)*bv->scale;
    y -= 5;
    if ( x<-400 || y<-40 || x>bv->width+400 || y>bv->height )
    {
        free(refinfo);
return;
    }

    GDrawLayoutInit(pixmap,refinfo,-1,bv->small);
    GDrawLayoutExtents(pixmap,&size);
    GDrawLayoutDraw(pixmap,x-size.width/2,y,fg);
    len = size.width;
    free(refinfo);
}

static void BVDrawGlyph(BitmapView *bv, BDFChar *bc, GWindow pixmap, GRect *pixel,
	uint8 is_ref, uint8 selected, int8 xoff, int8 yoff) {
    int i, j;
    int color = 0x808080;
    BDFFont *bdf = bv->bdf;
    BDFRefChar *cur;

    if ( is_ref && !selected  )
	GDrawSetStippled(pixmap,1, 0,0);
    for ( i=bc->ymax-bc->ymin; i>=0; --i ) {
	for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
	    pixel->x = bv->xoff + (bc->xmin + xoff +j)*bv->scale;
	    pixel->y = bv->height-bv->yoff - (bc->ymax + yoff - i + 1)*bv->scale;
	    if ( bdf->clut==NULL ) {
		if ( bc->bitmap[i*bc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))) ) {
		    GDrawFillRect(pixmap,pixel,color);
		    if ( selected ) {
			GDrawSetStippled(pixmap,2, 0,0);
			GDrawFillRect(pixmap,pixel,0x909000);
			GDrawSetStippled(pixmap,0, 0,0);
		    }
		}
	    } else {
		int index = bc->bitmap[i*bc->bytes_per_line+j];
		if ( index!=0 ) {
		    GDrawFillRect(pixmap,pixel,bdf->clut->clut[index]);
		    if ( selected ) {
			GDrawSetStippled(pixmap,2, 0,0);
			GDrawFillRect(pixmap,pixel,0x909000);
			GDrawSetStippled(pixmap,0, 0,0);
		    }
		}
	    }
	}
    }
    if ( is_ref ) {
	GDrawSetStippled(pixmap,0, 0,0);
	BVDrawRefBorder( bv,bc,pixmap,selected,xoff,yoff );
    }
    for ( cur=bc->refs; cur!=NULL; cur=cur->next ) if ( cur->bdfc != NULL ) {
	BVDrawGlyph( bv,cur->bdfc,pixmap,pixel,true,(cur->selected | selected),
	    xoff+cur->xoff,yoff+cur->yoff );
    }
}

static void BVExpose(BitmapView *bv, GWindow pixmap, GEvent *event ) {
    CharView cvtemp;
    GRect old;
    DRect clip;
    int i;
    GRect pixel;
    BDFChar *bc = bv->bc;
    BDFRefChar *bref;
    RefChar *refs;
    extern Color widthcol;

    GDrawPushClip(pixmap,&event->u.expose.rect,&old);
    GDrawSetLineWidth(pixmap,0);
    if ( bv->showfore ) {
	/* fore ground is a misnomer. it's what we're interested in but we */
	/*  actually need to draw it first, otherwise it obscures everything */
	pixel.width = pixel.height = bv->scale+1;
	BVDrawGlyph( bv,bc,pixmap,&pixel,false,false,0,0 );

	if ( bv->active_tool!=bvt_none ) {
	    /* This does nothing for many tools, but for lines, rects and circles */
	    /*  it draws temporary points */
	    BCGeneralFunction(bv,BVDrawTempPoint,pixmap);
	}
	/* Selected references are handled in BVDrawGlyph() */
	if ( bv->bc->selection )
	    BVDrawSelection(bv,pixmap);
    }
    if ( bv->showgrid ) {
	if ( bv->scale>2 ) {
	    for ( i=bv->xoff+bv->scale; i<bv->width; i += bv->scale )
		GDrawDrawLine(pixmap,i,0, i,bv->height,0xa0a0a0);
	    for ( i=bv->xoff-bv->scale; i>0; i -= bv->scale )
		GDrawDrawLine(pixmap,i,0, i,bv->height,0xa0a0a0);
	    for ( i=-bv->yoff+bv->height-bv->scale; i>0; i -= bv->scale )
		GDrawDrawLine(pixmap,0,i,bv->width,i,0xa0a0a0);
	    for ( i=-bv->yoff+bv->height+bv->scale; i<bv->height; i += bv->scale )
		GDrawDrawLine(pixmap,0,i,bv->width,i,0xa0a0a0);
	}
	GDrawDrawLine(pixmap,0,-bv->yoff+bv->height-0*bv->scale,bv->width,-bv->yoff+bv->height-0*bv->scale,0x404040);
	GDrawDrawLine(pixmap,0,-bv->yoff+bv->height-bv->bdf->ascent*bv->scale,
		bv->width,-bv->yoff+bv->height-bv->bdf->ascent*bv->scale,0x404040);
	GDrawDrawLine(pixmap,0,-bv->yoff+bv->height+bv->bdf->descent*bv->scale,
		bv->width,-bv->yoff+bv->height+bv->bdf->descent*bv->scale,0x404040);
	GDrawDrawLine(pixmap,bv->xoff+0*bv->scale,0, bv->xoff+0*bv->scale,bv->height,0x404040);
	GDrawDrawLine(pixmap,bv->xoff+bv->bc->width*bv->scale,0, bv->xoff+bv->bc->width*bv->scale,bv->height,widthcol);
	if ( bv->bdf->sf->hasvmetrics )
	    GDrawDrawLine(pixmap,0,-bv->yoff+bv->height-(bv->bdf->ascent-bc->vwidth)*bv->scale,
		    bv->width,-bv->yoff+bv->height-(bv->bdf->ascent-bc->vwidth)*bv->scale,widthcol);
    }
    if ( bv->showfore ) {
	/* Reference names are drawn after grid (otherwise some characters may get unreadable */
	for ( bref=bc->refs; bref!=NULL; bref=bref->next )
	    BVDrawRefName( bv,pixmap,bref,0 );
    }
    if ( bv->showoutline ) {
	Color col = (view_bgcol<0x808080)
		? (bv->bc->byte_data ? 0x008800 : 0x004400 )
		: 0x00ff00;
	memset(&cvtemp,'\0',sizeof(cvtemp));
	cvtemp.v = bv->v;
	cvtemp.width = bv->width;
	cvtemp.height = bv->height;
	cvtemp.scale = bv->scscale*bv->scale;
	cvtemp.xoff = bv->xoff/* *bv->scscale*/;
	cvtemp.yoff = bv->yoff/* *bv->scscale*/;
	cvtemp.b.sc = bv->bc->sc;
	cvtemp.b.drawmode = dm_fore;

	clip.width = event->u.expose.rect.width/cvtemp.scale;
	clip.height = event->u.expose.rect.height/cvtemp.scale;
	clip.x = (event->u.expose.rect.x-cvtemp.xoff)/cvtemp.scale;
	clip.y = (cvtemp.height-event->u.expose.rect.y-event->u.expose.rect.height-cvtemp.yoff)/cvtemp.scale;
	CVDrawSplineSet(&cvtemp,pixmap,cvtemp.b.sc->layers[ly_fore].splines,col,false,&clip);
	for ( refs = cvtemp.b.sc->layers[ly_fore].refs; refs!=NULL; refs = refs->next )
	    CVDrawSplineSet(&cvtemp,pixmap,refs->layers[0].splines,col,false,&clip);
    }
    if ( bv->active_tool==bvt_pointer ) {
	if ( bv->bc->selection==NULL ) {
	    int xmin, xmax, ymin, ymax;
	    xmin = bv->pressed_x; xmax = bv->info_x;
	    ymin = bv->info_y; ymax = bv->pressed_y;
	    if ( ymin>ymax ) { ymax = ymin; ymin = bv->pressed_y; }
	    if ( xmin>xmax ) { xmin = xmax; xmax = bv->pressed_x; }
	    pixel.width = (xmax-xmin+1) * bv->scale;
	    pixel.height = (ymax-ymin+1) * bv->scale;
	    pixel.x =  bv->xoff + xmin*bv->scale;
	    pixel.y = bv->height-bv->yoff-(ymax+1)*bv->scale;
	    GDrawSetDashedLine(pixmap,3,3,0);
	    GDrawDrawRect(pixmap,&pixel,0xffffff);
	    GDrawSetDashedLine(pixmap,3,3,3);
	    GDrawDrawRect(pixmap,&pixel,0x000000);
	    GDrawSetDashedLine(pixmap,0,0,0);
	}
    }
    GDrawPopClip(pixmap,&old);
}

static void BVInfoDrawText(BitmapView *bv, GWindow pixmap ) {
    GRect r;
    Color bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
    char buffer[50];
    int ybase = bv->mbh+10+bv->sas;

    GDrawSetFont(pixmap,bv->small);
    r.x = bv->infoh+RPT_DATA; r.width = 39;
    r.y = bv->mbh; r.height = 36 /* bv->infoh-1 */;
    GDrawFillRect(pixmap,&r,bg);

    sprintf(buffer,"%d%s%d", bv->info_x, coord_sep, bv->info_y );
    buffer[11] = '\0';
    GDrawDrawText8(pixmap,bv->infoh+RPT_DATA,ybase,buffer,-1,GDrawGetDefaultForeground(NULL));

    if ( bv->active_tool!=cvt_none ) {
	sprintf(buffer,"%d%s%d", bv->info_x-bv->pressed_x, coord_sep, bv->info_y-bv->pressed_y );
	buffer[11] = '\0';
	GDrawDrawText8(pixmap,bv->infoh+RPT_DATA,ybase+bv->sfh+10,buffer,-1,GDrawGetDefaultForeground(NULL));
    }
}

static void BVMainExpose(BitmapView *bv, GWindow pixmap, GEvent *event ) {
    GRect old, temp, box, old2, r;
    GImage gi;
    struct _GImage base;
    GClut clut;
    BDFChar *bdfc = BDFGetMergedChar( bv->bc );

    temp = event->u.expose.rect;
    if ( temp.y+temp.height < bv->mbh )
return;
    if ( temp.y <bv->mbh ) {
	temp.height -= (bv->mbh-temp.y);
	temp.y = bv->mbh;
    }
    GDrawPushClip(pixmap,&temp,&old);
    GDrawSetLineWidth(pixmap,0);

    if ( event->u.expose.rect.x<6+bdfc->xmax-bdfc->xmin ) {
	box.x = 0; box.width = bv->infoh;
	box.y = bv->mbh; box.height = bv->infoh;
	GDrawPushClip(pixmap,&box,&old2);

	memset(&gi,'\0',sizeof(gi));
	memset(&base,'\0',sizeof(base));
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	if ( bv->bdf->clut==NULL ) {
	    base.image_type = it_mono;
	    base.clut = &clut;
	    clut.clut_len = 2;
	    clut.clut[0] = GDrawGetDefaultBackground(NULL);
	} else {
	    base.image_type = it_index;
	    base.clut = bv->bdf->clut;
	}
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	GDrawDrawImage(pixmap,&gi,NULL, 5,bv->mbh+(bv->infoh-base.height)/2);

	GDrawPopClip(pixmap,&old2);

	GDrawDrawImage(pixmap,&GIcon_rightpointer,NULL,bv->infoh+RPT_BASE,bv->mbh+8);
	GDrawDrawImage(pixmap,&GIcon_press2ptr,NULL,bv->infoh+RPT_BASE,bv->mbh+18+bv->sfh);
	BVInfoDrawText(bv,pixmap );

	r.x = bv->infoh+RPT_DATA; r.y = bv->mbh+36;
	r.width = 20; r.height = 10;
	GDrawFillRect(pixmap,&r,
		bv->bdf->clut==NULL ? GDrawGetDefaultBackground(NULL) :
		bv->bdf->clut->clut[bv->color/( 255/((1<<BDFDepth(bv->bdf))-1) )] );

	GDrawDrawImage(pixmap,&GIcon_press2ptr,NULL,bv->infoh+RPT_BASE,bv->mbh+18+bv->sfh);
    }
    GDrawDrawLine(pixmap,0,bv->mbh+bv->infoh-1,bv->width+300,bv->mbh+bv->infoh-1,GDrawGetDefaultForeground(NULL));

    r.x = bv->width; r.y = bv->height+bv->infoh+bv->mbh;
    LogoExpose(pixmap,event,&r,dm_fore);

    GDrawPopClip(pixmap,&old);
    BDFCharFree( bdfc );
}

static void BVShowInfo(BitmapView *bv) {
    BVInfoDrawText(bv,bv->gw );
}

static void BVResize(BitmapView *bv, GEvent *event ) {
    int sbsize = GDrawPointsToPixels(bv->gw,_GScrollBar_Width);
    int newwidth = event->u.resize.size.width-sbsize,
	newheight = event->u.resize.size.height-sbsize - bv->mbh-bv->infoh;
    GRect size;

    if ( newwidth == bv->width && newheight == bv->height )
return;

    /* MenuBar takes care of itself */
    GDrawResize(bv->v,newwidth,newheight);
    GGadgetMove(bv->vsb,newwidth, bv->mbh+bv->infoh);
    GGadgetResize(bv->vsb,sbsize,newheight);
    GGadgetMove(bv->hsb,0,event->u.resize.size.height-sbsize);
    GGadgetResize(bv->hsb,newwidth,sbsize);
    bv->width = newwidth; bv->height = newheight;
    GGadgetGetSize(bv->recalc,&size);
    GGadgetMove(bv->recalc,event->u.resize.size.width - size.width - GDrawPointsToPixels(bv->gw,6),size.y);
    GDrawRequestExpose(bv->gw,NULL,false);
    BVFit(bv);

    bv_width  = event->u.resize.size.width;
    bv_height = event->u.resize.size.height;
    SavePrefs(true);
}

static void BVHScroll(BitmapView *bv,struct sbevent *sb) {
    int newpos = bv->xoff;
    int fh = bv->bdf->ascent+bv->bdf->descent;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos += 9*bv->width/10;
      break;
      case et_sb_up:
        newpos += bv->width/15;
      break;
      case et_sb_down:
        newpos -= bv->width/15;
      break;
      case et_sb_downpage:
        newpos -= 9*bv->width/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = -sb->pos;
      break;
      case et_sb_halfup:
        newpos += bv->width/30;
      break;
      case et_sb_halfdown:
        newpos -= bv->width/30;
      break;
    }
    if ( newpos>6*fh*bv->scale-bv->width )
        newpos = 6*fh*bv->scale-bv->width;
    if ( newpos<-3*fh*bv->scale ) newpos = -3*fh*bv->scale;
    if ( newpos!=bv->xoff ) {
	int diff = newpos-bv->xoff;
	bv->xoff = newpos;
	GScrollBarSetPos(bv->hsb,-newpos);
	GDrawScroll(bv->v,NULL,diff,0);
    }
}

static void BVVScroll(BitmapView *bv,struct sbevent *sb) {
    int newpos = bv->yoff;
    int fh = bv->bdf->ascent+bv->bdf->descent;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*bv->width/10;
      break;
      case et_sb_up:
        newpos -= bv->width/15;
      break;
      case et_sb_down:
        newpos += bv->width/15;
      break;
      case et_sb_downpage:
        newpos += 9*bv->width/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
      case et_sb_halfup:
        newpos -= bv->width/30;
      break;
      case et_sb_halfdown:
        newpos += bv->width/30;
      break;
    }
    if ( newpos>4*fh*bv->scale-bv->height )
        newpos = 4*fh*bv->scale-bv->height;
    if ( newpos<-2*fh*bv->scale ) newpos = -2*fh*bv->scale;
    if ( newpos!=bv->yoff ) {
	int diff = newpos-bv->yoff;
	bv->yoff = newpos;
	GScrollBarSetPos(bv->vsb,newpos);
	GDrawScroll(bv->v,NULL,0,diff);
    }
}

static int BVRecalc(GGadget *g, GEvent *e) {
    BitmapView *bv;
    BDFChar *bdfc;
    void *freetypecontext=NULL;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	bv = GDrawGetUserData(GGadgetGetWindow(g));
	BCPreserveState(bv->bc);
	BCFlattenFloat(bv->bc);
	freetypecontext = FreeTypeFontContext(bv->bc->sc->parent,bv->bc->sc,NULL,ly_fore);
	if ( freetypecontext!=NULL ) {
	    bdfc = SplineCharFreeTypeRasterize(freetypecontext,bv->bc->sc->orig_pos,bv->bdf->pixelsize,72,BDFDepth(bv->bdf));
	    FreeTypeFreeContext(freetypecontext);
	} else
	    bdfc = SplineCharAntiAlias(bv->bc->sc,ly_fore,bv->bdf->pixelsize,(1<<(BDFDepth(bv->bdf)/2)));
	free(bv->bc->bitmap);
	bv->bc->bitmap = bdfc->bitmap; bdfc->bitmap = NULL;
	bv->bc->width = bdfc->width;
	bv->bc->xmin = bdfc->xmin;
	bv->bc->xmax = bdfc->xmax;
	bv->bc->ymin = bdfc->ymin;
	bv->bc->ymax = bdfc->ymax;
	bv->bc->bytes_per_line = bdfc->bytes_per_line;
	BDFCharFree(bdfc);
	BCCharChangedUpdate(bv->bc);
    }
return( true );
}

static void BVSetWidth(BitmapView *bv, int x) {
    int tot, cnt;
    BDFFont *bdf;
    BDFChar *bc = bv->bc;

    bc->width = x;
    if ( bv->bdf->sf->onlybitmaps ) {
	tot=0; cnt=0;
	for ( bdf = bv->bdf->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->glyphs[bc->orig_pos]) {
		tot += bdf->glyphs[bc->orig_pos]->width*1000/(bdf->ascent+bdf->descent);
		++cnt;
	    }
	if ( cnt!=0 ) {
	    bc->sc->width = tot/cnt;
	    bc->sc->widthset = true;
	}
    }
    BCCharChangedUpdate(bc);
}

static void BVSetVWidth(BitmapView *bv, int y) {
    int tot, cnt;
    BDFFont *bdf;
    BDFChar *bc = bv->bc;

    if ( !bv->bdf->sf->hasvmetrics )
return;
    bc->vwidth = bv->bdf->ascent-y;
    if ( bv->bdf->sf->onlybitmaps ) {
	tot=0; cnt=0;
	for ( bdf = bv->bdf->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->glyphs[bc->orig_pos]) {
		tot += bdf->glyphs[bc->orig_pos]->vwidth*1000/(bdf->ascent+bdf->descent);
		++cnt;
	    }
	if ( cnt!=0 && bv->bdf->sf->onlybitmaps ) {
	    bc->sc->vwidth = tot/cnt;
	    bc->sc->widthset = true;
	}
    }
    BCCharChangedUpdate(bc);
}

int BVColor(BitmapView *bv) {
    int div = 255/((1<<BDFDepth(bv->bdf))-1);
return ( (bv->color+div/2)/div );
}

static int IsReferenceTouched(BitmapView *bv, BDFRefChar *ref, int x, int y){
    BDFRefChar *head;
    BDFChar *rbc;
    int nx, ny;

    if ( ref == NULL )
return( false );

    rbc = ref->bdfc;
    ny = rbc->ymax - y + ref->yoff;
    nx = x - rbc->xmin - ref->xoff;
    if (nx>=0 && nx<=( rbc->xmax - rbc->xmin ) &&
	ny>=0 && ny<=( rbc->ymax - rbc->ymin ) &&
	(( rbc->byte_data && rbc->bitmap[ny*rbc->bytes_per_line+nx] != 0 ) ||
	(( !rbc->byte_data &&
	    rbc->bitmap[ny*rbc->bytes_per_line + (nx>>3)] & (1<<(7 - (nx&7)))))))
return( true );

    for ( head = rbc->refs; head != NULL; head = head->next ) {
	if ( IsReferenceTouched( bv,head,x,y))
return( true );
    }
return( false );
}

static void BVMouseDown(BitmapView *bv, GEvent *event) {
    int x = floor( (event->u.mouse.x-bv->xoff)/ (real) bv->scale);
    int y = floor( (bv->height-event->u.mouse.y-bv->yoff)/ (real) bv->scale);
    int ny;
    BDFChar *bc = bv->bc;
    BDFFloat *sel;
    int color_under_cursor;
    BDFRefChar *refsel = NULL, *head;

    if ( event->u.mouse.button==2 && event->u.mouse.device!=NULL &&
	    strcmp(event->u.mouse.device,"stylus")==0 )
return;		/* I treat this more like a modifier key change than a button press */

    if ( event->u.mouse.button==3 ) {
	BVToolsPopup(bv,event);
return;
    }
    BVToolsSetCursor(bv,event->u.mouse.state|(1<<(7+event->u.mouse.button)), event->u.mouse.device );
    bv->active_tool = bv->showing_tool;
    bv->pressed_x = x; bv->pressed_y = y;
    bv->info_x = x; bv->info_y = y;
    ny = bc->ymax-y;
    if ( x<bc->xmin || x>bc->xmax || ny<0 || ny>bc->ymax-bc->ymin )
	color_under_cursor = 0;
    else if ( bc->byte_data )
	color_under_cursor = bc->bitmap[(bc->ymax-y)*bc->bytes_per_line + x-bc->xmin] *
		255/((1<<BDFDepth(bv->bdf))-1);
    else
	color_under_cursor = bc->bitmap[(bc->ymax-y)*bc->bytes_per_line + (x-bc->xmin)/8]&(0x80>>((x-bc->xmin)&7)) *
		255;
    BVPaletteColorUnderChange(bv,color_under_cursor);
    bv->event_x = event->u.mouse.x; bv->event_y = event->u.mouse.y;
    bv->recentchange = false;
    for ( head = bc->refs; head != NULL; head = head->next ) {
	if ( IsReferenceTouched( bv,head,x,y ))
	    refsel = head;
    }
    switch ( bv->active_tool ) {
      case bvt_eyedropper:
	bv->color = color_under_cursor;
	/* Store color as a number between 0 and 255 no matter what the clut size is */
	BVPaletteColorChange(bv);
      break;
      case bvt_pencil: case bvt_line:
      case bvt_rect: case bvt_filledrect:
	ny = bc->ymax-y;
	bv->clearing = false;
	if ( !bc->byte_data && x>=bc->xmin && x<=bc->xmax &&
		ny>=0 && ny<=bc->ymax-bc->ymin ) {
	    int nx = x-bc->xmin;
	    if ( bc->bitmap[ny*bc->bytes_per_line + (nx>>3)] &
		(1<<(7-(nx&7))) )
	    bv->clearing = true;
	}
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	if ( bv->active_tool == bvt_pencil )
	    BCSetPoint(bc,x,y,bc->byte_data?BVColor(bv):!bv->clearing);
	BCCharChangedUpdate(bc);
      break;
      case bvt_elipse: case bvt_filledelipse:
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	BCCharChangedUpdate(bc);
      break;
      case bvt_pointer:
	if ( !( event->u.mouse.state&ksm_shift )) {
	    for ( head = bc->refs; head != NULL; head=head->next )
		head->selected = false;
	}
	if ( refsel != NULL ) {
	    BCFlattenFloat(bc);
	    refsel->selected = ( event->u.mouse.state&ksm_shift ) ?
		!(refsel->selected) : true;
	    GDrawSetCursor(bv->v,ct_shift);
	} else if ( (sel = bc->selection)!=NULL ) {
	    if ( x<sel->xmin || x>sel->xmax || y<sel->ymin || y>sel->ymax )
		BCFlattenFloat(bc);
	    else {
		GDrawSetCursor(bv->v,ct_shift);
		/* otherwise we'll move the selection */
	    }
	} else if ( /*bc->sc->parent->onlybitmaps &&*/
		event->u.mouse.x-bv->xoff > bc->width*bv->scale-3 &&
		event->u.mouse.x-bv->xoff < bc->width*bv->scale+3 ) {
	    bv->active_tool = bvt_setwidth;
	    BVToolsSetCursor(bv,event->u.mouse.state|(1<<(7+event->u.mouse.button)), event->u.mouse.device );
	} else if ( /*bc->sc->parent->onlybitmaps &&*/ bc->sc->parent->hasvmetrics &&
		bv->height-event->u.mouse.y-bv->yoff > (bv->bdf->ascent-bc->vwidth)*bv->scale-3 &&
		bv->height-event->u.mouse.y-bv->yoff < (bv->bdf->ascent-bc->vwidth)*bv->scale+3 ) {
	    bv->active_tool = bvt_setvwidth;
	    BVToolsSetCursor(bv,event->u.mouse.state|(1<<(7+event->u.mouse.button)), event->u.mouse.device );
	}
	BCCharUpdate(bc);
      break;
      case bvt_setwidth:
	BVSetWidth(bv,x);
      break;
      case bvt_setvwidth:
	BVSetVWidth(bv,y);
      break;
    }
}

static void BVMouseMove(BitmapView *bv, GEvent *event) {
    int x = floor( (event->u.mouse.x-bv->xoff)/ (real) bv->scale);
    int y = floor( (bv->height-event->u.mouse.y-bv->yoff)/ (real) bv->scale);
    int newx, newy;
    int fh = bv->bdf->ascent+bv->bdf->descent;
    BDFChar *bc = bv->bc;
    int color_under_cursor, ny, has_selected_refs = false;
    BDFRefChar *head;

    bv->info_x = x; bv->info_y = y;
    ny = bc->ymax-y;
    if ( x<bc->xmin || x>bc->xmax || ny<0 || ny>bc->ymax-bc->ymin )
	color_under_cursor = 0;
    else if ( bc->byte_data )
	color_under_cursor = bc->bitmap[(bc->ymax-y)*bc->bytes_per_line + x-bc->xmin] *
		255/((1<<BDFDepth(bv->bdf))-1);
    else
	color_under_cursor = bc->bitmap[(bc->ymax-y)*bc->bytes_per_line + (x-bc->xmin)/8]&(0x80>>((x-bc->xmin)&7)) *
		255;
    BVShowInfo(bv);
    BVPaletteColorUnderChange(bv,color_under_cursor);
    if ( bv->active_tool==bvt_none )
return;			/* Not pressed */
    switch ( bv->active_tool ) {
      case bvt_pencil:
	BCSetPoint(bc,x,y,bc->byte_data?BVColor(bv):!bv->clearing);
	BCCharChangedUpdate(bc);
      break;
      case bvt_line: case bvt_rect: case bvt_filledrect:
      case bvt_elipse: case bvt_filledelipse:
	BCCharChangedUpdate(bc);
      break;
      case bvt_hand:
	newx = bv->xoff + event->u.mouse.x-bv->event_x;
	newy = bv->yoff + bv->event_y-event->u.mouse.y;
	if ( newy>4*fh*bv->scale-bv->height )
	    newy = 4*fh*bv->scale-bv->height;
	if ( newy<-2*fh*bv->scale ) newy = -2*fh*bv->scale;
	if ( newx>6*fh*bv->scale-bv->width )
	    newx = 6*fh*bv->scale-bv->width;
	if ( newx<-3*fh*bv->scale ) newx = -3*fh*bv->scale;
	if ( newx!=bv->xoff || newy!=bv->yoff ) {
	    newx -= bv->xoff; bv->xoff += newx;
	    newy -= bv->yoff; bv->yoff += newy;
	    GScrollBarSetPos(bv->hsb,-bv->xoff);
	    GScrollBarSetPos(bv->vsb,-bv->yoff);
	    GDrawScroll(bv->v,NULL,newx,newy);
	}
	bv->event_x = event->u.mouse.x; bv->event_y = event->u.mouse.y;
      break;
      case bvt_shift:
	if ( x!=bv->pressed_x || y!=bv->pressed_y ) {
	    if ( !bv->recentchange ) {
		BCPreserveState(bc);
		BCFlattenFloat(bc);
		bv->recentchange = true;
	    }
	    bc->xmin += x-bv->pressed_x;
	    bc->xmax += x-bv->pressed_x;
	    bc->ymin += y-bv->pressed_y;
	    bc->ymax += y-bv->pressed_y;

	    for ( head=bc->refs; head!=NULL; head=head->next ) {
		if ( head->selected ) {
		    head->xoff += x-bv->pressed_x;
		    head->yoff += y-bv->pressed_y;
		}
	    }
	    BCCharChangedUpdate(bc);
	    bv->pressed_x = x; bv->pressed_y = y;
	}
      break;
      case bvt_pointer:
	for ( head=bc->refs; head!=NULL && !has_selected_refs; head=head->next ) {
	    if ( head->selected ) has_selected_refs = true;
	}
	if ( bc->selection!=NULL || has_selected_refs ) {
	    if ( x!=bv->pressed_x || y!=bv->pressed_y ) {
		if ( !bv->recentchange ) {
		    BCPreserveState(bc);
		    bv->recentchange = true;
		}
		if ( has_selected_refs ) {
		    for ( head=bc->refs; head!=NULL; head=head->next ) {
			if ( head->selected ) {
			    head->xoff += x-bv->pressed_x;
			    head->yoff += y-bv->pressed_y;
			}
		    }
		} else if ( bc->selection != NULL ) {
		    bc->selection->xmin += x-bv->pressed_x;
		    bc->selection->xmax += x-bv->pressed_x;
		    bc->selection->ymin += y-bv->pressed_y;
		    bc->selection->ymax += y-bv->pressed_y;
		}
		BCCharChangedUpdate(bc);
		bv->pressed_x = x; bv->pressed_y = y;
	    }
	} else {
	    GDrawRequestExpose(bv->v,NULL,false);
	}
      break;
      case bvt_setwidth:
	BVSetWidth(bv,x);
      break;
      case bvt_setvwidth:
	BVSetVWidth(bv,y);
      break;
    }
}

static void BVSetPoint(BitmapView *bv, int x, int y, void *junk) {
    BCSetPoint(bv->bc,x,y,bv->bc->byte_data?BVColor(bv):!bv->clearing);
}

static void BVMagnify(BitmapView *bv, int midx, int midy, int bigger);

static void BVMouseUp(BitmapView *bv, GEvent *event) {
    int x = floor( (event->u.mouse.x-bv->xoff)/ (real) bv->scale);
    int y = floor( (bv->height-event->u.mouse.y-bv->yoff)/ (real) bv->scale);
    BDFRefChar *refsel = NULL, *head;

    BVMouseMove(bv,event);
    for ( head = bv->bc->refs; head != NULL; head = head->next ) {
	if ( IsReferenceTouched( bv,head,x,y ))
	    refsel = head;
    }
    switch ( bv->active_tool ) {
      case bvt_magnify: case bvt_minify:
	BVMagnify(bv,x,y,bv->active_tool==bvt_magnify?1:-1);
      break;
      case bvt_line: case bvt_rect: case bvt_filledrect:
      case bvt_elipse: case bvt_filledelipse:
	if ( refsel == NULL ) {
	    BCGeneralFunction(bv,BVSetPoint,NULL);
	    bv->active_tool = bvt_none;
	    BCCharChangedUpdate(bv->bc);
	}
      break;
      case bvt_pointer:
	if ( bv->bc->selection!=NULL ) {
	    /* we've been moving it */
	    GDrawSetCursor(bv->v,ct_mypointer);
	    if ( !bv->recentchange ) {	/* Oh, we just clicked in it, get rid of it */
		BCFlattenFloat(bv->bc);
		BCCharChangedUpdate(bv->bc);
	    }
	} else if ( refsel ) {
	    GDrawSetCursor(bv->v,ct_mypointer);
	    bv->active_tool = bvt_none;
	    BCCharChangedUpdate(bv->bc);
	} else {
	    int dx,dy;
	    if ( (dx = event->u.mouse.x-bv->event_x)<0 ) dx = -dx;
	    if ( (dy = event->u.mouse.y-bv->event_y)<0 ) dy = -dy;
	    if ( dx+dy>4 ) {
		/* we've just dragged out a new one */
		BDFFloatCreate(bv->bc,bv->pressed_x,bv->info_x,bv->pressed_y,bv->info_y,true);
	    }
	    bv->active_tool = bvt_none;
	    BCCharChangedUpdate(bv->bc);
	}
      break;
      case bvt_setwidth:
	BVSetWidth(bv,x);
      break;
      case bvt_setvwidth:
	BVSetVWidth(bv,y);
      break;
    }
    bv->active_tool = bvt_none;
    BVToolsSetCursor(bv,event->u.mouse.state&~(1<<(7+event->u.mouse.button)), event->u.mouse.device);		/* X still has the buttons set in the state, even though we just released them. I don't want em */
}

static int v_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	int ish = event->u.mouse.button>5;
	if ( event->u.mouse.state&ksm_shift ) ish = !ish;
	if ( ish ) /* bind shift to vertical scrolling */
return( GGadgetDispatchEvent(bv->hsb,event));
	else
return( GGadgetDispatchEvent(bv->vsb,event));
    }

    switch ( event->type ) {
      case et_selclear:
	ClipboardClear();
      break;
      case et_expose:
	GDrawSetLineWidth(gw,0);
	BVExpose(bv,gw,event);
      break;
      case et_crossing:
	BVToolsSetCursor(bv,event->u.mouse.state, event->u.mouse.device);
      break;
      case et_mousedown:
	BVPaletteActivate(bv);
	BVMouseDown(bv,event);
      break;
      case et_mousemove:
	BVMouseMove(bv,event);
      break;
      case et_mouseup:
	BVMouseUp(bv,event);
      break;
      case et_char:
	BVChar(bv,event);
      break;
      case et_charup:
	BVCharUp(bv,event);
      break;
      case et_timer:
#if _ModKeysAutoRepeat
	/* Under cygwin the modifier keys auto repeat, they don't under normal X */
	if ( bv->autorpt==event->u.timer.timer ) {
	    bv->autorpt = NULL;
	    BVToolsSetCursor(bv,bv->oldstate,NULL);
	}
#endif
      break;
      case et_focus:
      break;
      default:
      break;
    }
return( true );
}

static int bv_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    int enc;

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	int ish = event->u.mouse.button>5;
	if ( event->u.mouse.state&ksm_shift ) ish = !ish;
	if ( ish ) /* bind shift to vertical scrolling */
return( GGadgetDispatchEvent(bv->hsb,event));
	else
return( GGadgetDispatchEvent(bv->vsb,event));
    }

    switch ( event->type ) {
      case et_expose:
	GDrawSetLineWidth(gw,0);
	BVMainExpose(bv,gw,event);
      break;
      case et_char:
	BVChar(bv,event);
      break;
      case et_charup:
	BVCharUp(bv,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    BVResize(bv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == bv->hsb )
		BVHScroll(bv,&event->u.control.u.sb);
	    else
		BVVScroll(bv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_destroy:
	BVUnlinkView(bv);
	BVPalettesHideIfMine(bv);
	BitmapViewFree(bv);
      break;
      case et_map:
	if ( event->u.map.is_visible )
	    BVPaletteActivate(bv);
	else
	    BVPalettesHideIfMine(bv);
      break;
      case et_close:
	GDrawDestroyWindow(gw);
      break;
      case et_mouseup: case et_mousedown:
	GGadgetEndPopup();
	BVPaletteActivate(bv);
      break;
      case et_mousemove:
	enc = BVCurEnc(bv);
	SCPreparePopup(bv->gw,bv->bc->sc,bv->fv->b.map->remap,enc,
		UniFromEnc(enc,bv->fv->b.map->enc));
      break;
      case et_focus:
      break;
    }
return( true );
}

#define MID_Fit		2001
#define MID_ZoomIn	2002
#define MID_ZoomOut	2003
#define MID_Next	2007
#define MID_Prev	2008
#define MID_Bigger	2009
#define MID_Smaller	2010
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_SelAll	2106
#define MID_CopyRef	2107
#define MID_UnlinkRef	2108
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_RemoveUndoes 2111
#define MID_GetInfo	2203
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Tools	2501
#define MID_Layers	2502
#define MID_Shades	2503
#define MID_DockPalettes	2504
#define MID_Revert	2702
#define MID_Recent	2703
#define MID_SetWidth	2601
#define MID_SetVWidth	2602

#define MID_Warnings	3000

static void BVMenuOpen(GWindow gw, struct gmenuitem *mi, GEvent *g) {
    BitmapView *d = (BitmapView*)GDrawGetUserData(gw);
    FontView *fv = NULL;
    if (d) {
        fv = (FontView*)d->fv;
    }
    _FVMenuOpen(fv);
}

static void BVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    GDrawDestroyWindow(gw);
}

static void BVMenuOpenOutline(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    CharViewCreate(bv->bc->sc,bv->fv,bv->map_of_enc==bv->fv->b.map?bv->enc:-1);
}

static void BVMenuOpenMetrics(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    MetricsViewCreate(bv->fv,bv->bc->sc,bv->bdf);
}

static void BVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    _FVMenuSave(bv->fv);
}

static void BVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    _FVMenuSaveAs(bv->fv);
}

static void BVMenuGenerate(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    _FVMenuGenerate(bv->fv,false);
}

static void BVMenuGenerateFamily(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    _FVMenuGenerate(bv->fv,gf_macfamily);
}

static void BVMenuGenerateTTC(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    _FVMenuGenerate(bv->fv,gf_ttc);
}

static void BVMenuExport(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BVExport(bv);
}

static void BVMenuImport(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BVImport(bv);
}

static void BVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    FVDelay(bv->fv,(void (*)(FontView *)) FVRevert);
			    /* The revert command can potentially */
			    /* destroy our window (if the char weren't in the */
			    /* old font). If that happens before the menu finishes */
			    /* we get a crash. So delay till after the menu completes */
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Revert:
	    mi->ti.disabled = bv->bdf->sf->origname==NULL || bv->bdf->sf->new;
	  break;
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static void BVMagnify(BitmapView *bv, int midx, int midy, int bigger) {
    /* available sizes: 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 */

    if ( bigger>0 ) {
	if ( bv->scale == 1 )
	    bv->scale = 2;
	else if ( (bv->scale & (bv->scale - 1)) == 0 )	 /* power of 2 */
	    bv->scale += bv->scale / 2;
	else
	    bv->scale += bv->scale / 3;
	if ( bv->scale > 32 ) bv->scale = 32;
    } else {
	if ( bv->scale <= 2 )
	    bv->scale = 1;
	else if ( (bv->scale & (bv->scale - 1)) == 0 )
	    bv->scale -= bv->scale / 4;
	else
	    bv->scale -= bv->scale / 3;
    }
    bv->xoff = -(midx*bv->scale - bv->width/2);
    bv->yoff = -(midy*bv->scale - bv->height/2);
    BVNewScale(bv);
}

static void BVMenuScale(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if ( mi->mid == MID_Fit ) {
	BVFit(bv);
    } else {
	real midx = (bv->width/2-bv->xoff)/bv->scale;
	real midy = (bv->height/2-bv->yoff)/bv->scale;
	BVMagnify(bv,midx,midy,mi->mid==MID_ZoomOut?-1:1);
    }
}

static void BVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    SplineFont *sf = bv->bc->sc->parent;
    EncMap *map = bv->fv->b.map;
    int pos = -1, gid;

    if ( mi->mid == MID_Next ) {
	pos = BVCurEnc(bv)+1;
    } else if ( mi->mid == MID_Prev ) {
	pos = BVCurEnc(bv)-1;
    } else if ( mi->mid == MID_NextDef ) {
	for ( pos = BVCurEnc(bv)+1; pos<map->enccount &&
		    ((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid]) ||
			    bv->bdf->glyphs[gid]==NULL) ;
		++pos );
	if ( pos==map->enccount )
return;
    } else if ( mi->mid == MID_PrevDef ) {
	for ( pos = BVCurEnc(bv)-1; pos>=0 &&
		    ((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid]) ||
			    bv->bdf->glyphs[gid]==NULL) ;
		    --pos );
	if ( pos<0 )
return;
    }
    if ( pos>=0 && pos<map->enccount )
	BVChangeChar(bv,pos,false);
}

static void BVMenuChangePixelSize(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BDFFont *best=NULL, *bdf;
    /* Bigger will find either a bigger pixelsize or a font with same pixelsize and greater depth */

    if ( mi->mid == MID_Bigger ) {
	best = bv->bdf->next;		/* I rely on the bitmap list being ordered */
    } else {
	for ( bdf=bv->bdf->sf->bitmaps; bdf!=NULL && bdf->next!=bv->bdf; bdf=bdf->next );
	best = bdf;
    }
    if ( best!=NULL && bv->bdf!=best ) {
	bv->bdf = best;
	bv->scscale = ((real) (best->pixelsize))/(best->sf->ascent+best->sf->descent);
	BVChangeChar(bv,bv->enc,true);
	BVShows.lastpixelsize = best->pixelsize;
    }
}

static void BVMenuGotoChar(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    int pos = GotoChar(bv->fv->b.sf,bv->fv->b.map,NULL);

    if ( pos!=-1 )
	BVChangeChar(bv,pos,false);
}

static void BVMenuFindInFontView(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    FVChangeChar(bv->fv,bv->bc->sc->orig_pos);
    GDrawSetVisible(bv->fv->gw,true);
    GDrawRaise(bv->fv->gw);
}

static void BVMenuPalettesDock(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    PalettesChangeDocking();
}

static void BVMenuPaletteShow(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    BVPaletteSetVisible(bv, mi->mid==MID_Tools?1:mi->mid==MID_Shades?2:0, !BVPaletteIsVisible(bv, mi->mid==MID_Tools?1:mi->mid==MID_Shades?2:0));
}

static void BVUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    if ( bv->bc->undoes==NULL )
return;
    BCDoUndo(bv->bc);
}

static SplineChar *SCofBV(BitmapView *bv) {
    SplineFont *sf = bv->bdf->sf;
    int i, cid = bv->bc->orig_pos;

    if ( sf->subfonts ) {
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( cid<sf->subfonts[i]->glyphcnt && sf->subfonts[i]->glyphs[cid]!=NULL )
return( sf->subfonts[i]->glyphs[cid] );

return( NULL );
    } else
return( sf->glyphs[cid] );
}

static void BVMenuSetWidth(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    char buffer[10];
    char *ret;
    BDFFont *bdf;
    int mysize = bv->bdf->pixelsize;
    SplineChar *sc;
    int val;

    if ( !bv->bdf->sf->onlybitmaps )
return;
    if ( mi->mid==MID_SetWidth ) {
	sprintf( buffer,"%d",bv->bc->width);
	ret = gwwv_ask_string(_("Set Width..."),buffer,_("Set Width..."));
    } else {
	sprintf( buffer,"%d",bv->bc->vwidth);
	ret = gwwv_ask_string(_("Set Vertical Width..."),buffer,_("Set Vertical Width..."));
    }
    if ( ret==NULL )
return;
    val = strtol(ret,NULL,10);
    free(ret);
    if ( val<0 )
return;
    if ( mi->mid==MID_SetWidth )
	bv->bc->width = val;
    else
	bv->bc->vwidth = val;
    BCCharChangedUpdate(bv->bc);
    for ( bdf=bv->bdf->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	if ( bdf->pixelsize > mysize )
return;
    if ( (sc=SCofBV(bv))!=NULL ) {
	if ( mi->mid==MID_SetWidth )
	    sc->width = val*(sc->parent->ascent+sc->parent->descent)/mysize;
	else
	    sc->vwidth = val*(sc->parent->ascent+sc->parent->descent)/mysize;
	SCCharChangedUpdate(sc,ly_none);
    }
}

static void BVRedo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    if ( bv->bc->redoes==NULL )
return;
    BCDoRedo(bv->bc);
}

static void _BVUnlinkRef(BitmapView *bv) {
    int anyrefs = false;
    BDFRefChar *ref, *next, *prev = NULL;

    if ( bv->bc->refs!=NULL ) {
	BCPreserveState(bv->bc);
	for ( ref=bv->bc->refs; ref!=NULL && !anyrefs; ref=ref->next )
	    if ( ref->selected ) anyrefs = true;
	for ( ref=bv->bc->refs; ref!=NULL ; ref=next ) {
	    next = ref->next;
	    if ( ref->selected || !anyrefs) {
		BCPasteInto( bv->bc,ref->bdfc,ref->xoff,ref->yoff,false,false );
		BCMergeReferences( bv->bc,ref->bdfc,ref->xoff,ref->yoff );
		BCRemoveDependent( bv->bc,ref );
	    } else
		prev = ref;
	}
	BCCharChangedUpdate(bv->bc);
    }
}

static void BVUnlinkRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    _BVUnlinkRef(bv);
}

static void BVRemoveUndoes(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    UndoesFree(bv->bc->undoes); bv->bc->undoes = NULL;
    UndoesFree(bv->bc->redoes); bv->bc->redoes = NULL;
}

static void BVCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BCCopySelected(bv->bc,bv->bdf->pixelsize,BDFDepth(bv->bdf));
}

static void BVCopyRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BCCopyReference(bv->bc,bv->bdf->pixelsize,BDFDepth(bv->bdf));
}

static void BVDoClear(BitmapView *bv) {
    BDFRefChar *head, *next;
    BDFChar *bc = bv->bc;
    int refs_changed = false;

    for ( head=bc->refs; head!=NULL; head=next ) {
	next = head->next;
	if ( head->selected ) {
	    if ( !refs_changed ) {
		BCPreserveState( bc );
		refs_changed = true;
	    }
	    BCRemoveDependent( bc,head );
	}
    }

    if ( bc->selection!=NULL ) {
	BCPreserveState( bc );
	BDFFloatFree( bc->selection );
	bv->bc->selection = NULL;
	BCCharChangedUpdate( bc );
    } else if ( refs_changed ) {
	BCCharChangedUpdate( bc );
    }
}

static void BVClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BVDoClear(bv);
}

static void BVPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    if ( CopyContainsBitmap())
	PasteToBC(bv->bc,bv->bdf->pixelsize,BDFDepth(bv->bdf));
}

static void BVCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BVCopy(gw,mi,e);
    BVDoClear(bv);
}

static void BVSelectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BDFChar *bc = bv->bc;

    BDFFloatCreate(bc,bc->xmin,bc->xmax,bc->ymin,bc->ymax, true);
    BCCharUpdate(bc);
}

static void BVMenuFontInfo(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    DelayEvent(FontMenuFontInfo,bv->fv);
}

static void BVMenuBDFInfo(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    SFBdfProperties(bv->bdf->sf,bv->fv->b.map,bv->bdf);
}

static void BVMenuGetInfo(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    SCCharInfo(bv->bc->sc,bv->fv->b.active_layer,bv->fv->b.map,BVCurEnc(bv));
}

static void BVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BitmapDlg(bv->fv,bv->bc->sc,mi->mid==MID_AvailBitmaps );
}

static void BVMenuRmGlyph(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BitmapView *bvs, *bvnext;
    BDFFont *bdf = bv->bdf;
    BDFChar *bc = bv->bc;
    FontView *fv;

    for ( bvs=bc->views; bvs!=NULL; bvs=bvnext ) {
	bvnext = bvs->next;
	GDrawDestroyWindow(bvs->gw);
    }
    bdf->glyphs[bc->orig_pos] = NULL;
    /* Can't free the glyph yet, need to process all the destroy events */
    /*  which touch bc->views first */
    DelayEvent( (void (*)(void *))BDFCharFree,bc);
    for ( fv = (FontView *) (bdf->sf->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame) )
	GDrawRequestExpose(fv->v,NULL,false);
}

static int askfraction(int *xoff, int *yoff) {
    static int lastx=1, lasty = 3;
    char buffer[30];
    char *ret, *end, *end2;
    int xv, yv;

    sprintf( buffer, "%d:%d", lastx, lasty );
    ret = ff_ask_string(_("Skew"),buffer,_("Skew Ratio"));
    if ( ret==NULL )
return( 0 );
    xv = strtol(ret,&end,10);
    yv = strtol(end+1,&end2,10);
    if ( xv==0 || xv>10 || xv<-10 || yv<=0 || yv>10 || *end!=':' || *end2!='\0' ) {
	ff_post_error( _("Bad Number"),_("Bad Number") );
	free(ret);
return( 0 );
    }
    free(ret);
    *xoff = lastx = xv; *yoff = lasty = yv;
return( 1 );
}

void BVRotateBitmap(BitmapView *bv,enum bvtools type ) {
    int xoff=0, yoff=0;

    if ( type==bvt_skew )
	if ( !askfraction(&xoff,&yoff))
return;
    BCPreserveState(bv->bc);
    BCTransFunc(bv->bc,type,xoff,yoff);
    BCCharChangedUpdate(bv->bc);
}

void BVMenuRotateInvoked(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BVRotateBitmap(bv,mi->mid);
}

static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_RegenBitmaps:
	    mi->ti.disabled = bv->bdf->sf->onlybitmaps;
	  break;
	}
    }
}

static void edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BDFRefChar *cur;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Cut: /*case MID_Copy:*/ case MID_Clear:
	    /* If nothing is selected, copy copies everything */
	    mi->ti.disabled = bv->bc->selection==NULL;
	    for ( cur=bv->bc->refs; cur!=NULL && mi->ti.disabled; cur=cur->next ) {
		if ( cur->selected )
		    mi->ti.disabled = false;
	    }
	  break;
	  case MID_Paste:
	    mi->ti.disabled = !CopyContainsBitmap();
	  break;
	  case MID_Undo:
	    mi->ti.disabled = bv->bc->undoes==NULL;
	  break;
	  case MID_Redo:
	    mi->ti.disabled = bv->bc->redoes==NULL;
	  break;
	  case MID_RemoveUndoes:
	    mi->ti.disabled = bv->bc->redoes==NULL && bv->bc->undoes==NULL;
	  break;
	  case MID_UnlinkRef:
	    mi->ti.disabled = bv->bc->refs==NULL;
	  break;
	}
    }
}

static void pllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    extern int palettes_docked;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Tools:
	    mi->ti.checked = BVPaletteIsVisible(bv,1);
	  break;
	  case MID_Layers:
	    mi->ti.checked = BVPaletteIsVisible(bv,0);
	  break;
	  case MID_Shades:
	    mi->ti.disabled = BDFDepth(bv->bdf)==1;
	    if ( !mi->ti.disabled )
		mi->ti.checked = BVPaletteIsVisible(bv,2);
	  break;
	  case MID_DockPalettes:
	    mi->ti.checked = palettes_docked;
	  break;
	}
    }
}

static void vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BDFFont *bdf;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_ZoomIn:
	    mi->ti.disabled = bv->scale==32;
	  break;
	  case MID_ZoomOut:
	    mi->ti.checked = bv->scale==1;
	  break;
	  case MID_Bigger:
	    mi->ti.disabled = bv->bdf->next==NULL;
	  break;
	  case MID_Smaller:
	    for ( bdf=bv->bdf->sf->bitmaps; bdf!=NULL && bdf->next!=bv->bdf; bdf=bdf->next );
	    mi->ti.disabled = bdf==NULL;
	  break;
	}
    }
}

static void mtlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_SetWidth:
	    mi->ti.disabled = !bv->bdf->sf->onlybitmaps;
	  break;
	  case MID_SetVWidth:
	    mi->ti.disabled = !bv->bdf->sf->onlybitmaps || !bv->bdf->sf->hasvmetrics;
	  break;
	}
    }
}

static GMenuItem2 wnmenu[] = {
    { { (unichar_t *) N_("New O_utline Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("New Outline Window|Ctl+H"), NULL, NULL, BVMenuOpenOutline, 0 },
    { { (unichar_t *) N_("New _Bitmap Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("New Bitmap Window|Ctl+J"), NULL, NULL, /* No function, never avail */NULL, 0 },
    { { (unichar_t *) N_("New _Metrics Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("New Metrics Window|Ctl+K"), NULL, NULL, BVMenuOpenMetrics, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Warnings"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Warnings|No Shortcut"), NULL, NULL, _MenuWarnings, MID_Warnings },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    GMENUITEM2_EMPTY
};

static void BVWindowMenuBuild(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct gmenuitem *wmi;
    WindowMenuBuild(gw,mi,e);
    for ( wmi = mi->sub; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case MID_Warnings:
	    wmi->ti.disabled = ErrorWindowExists();
	  break;
	}
    }
}

static void BVMenuContextualHelp(GWindow base,struct gmenuitem *mi,GEvent *e) {
    help("bitmapview.html");
}

char *BVFlipNames[] = { N_("Flip Horizontally"), N_("Flip Vertically"),
/* GT: "CW" means Clockwise */
    NU_("Rotate 90° CW"),
/* GT: "CW" means Counter-Clockwise */
    NU_("Rotate 90° CCW"),
    NU_("Rotate 180°"),
    N_("Skew..."), NULL };

static GMenuItem2 dummyitem[] = {
    { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, NULL, NULL, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};
static GMenuItem2 fllist[] = {
    { { (unichar_t *) N_("Font|_New"), (GImage *) "filenew.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("New|Ctl+N"), NULL, NULL, MenuNew, 0 },
    { { (unichar_t *) N_("_Open"), (GImage *) "fileopen.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Open|Ctl+O"), NULL, NULL, BVMenuOpen, 0 },
    { { (unichar_t *) N_("Recen_t"), (GImage *) "filerecent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, NULL, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) N_("_Close"), (GImage *) "fileclose.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Close|Ctl+Shft+Q"), NULL, NULL, BVMenuClose, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Save"), (GImage *) "filesave.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Save|Ctl+S"), NULL, NULL, BVMenuSave, 0 },
    { { (unichar_t *) N_("S_ave as..."), (GImage *) "filesaveas.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Save as...|Ctl+Shft+S"), NULL, NULL, BVMenuSaveAs, 0 },
    { { (unichar_t *) N_("_Generate Fonts..."), (GImage *) "filegenerate.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Generate Fonts...|Ctl+Shft+G"), NULL, NULL, BVMenuGenerate, 0 },
    { { (unichar_t *) N_("Generate Mac _Family..."), (GImage *) "filegeneratefamily.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Generate Mac Family...|Alt+Ctl+G"), NULL, NULL, BVMenuGenerateFamily, 0 },
    { { (unichar_t *) N_("Generate TTC..."), (GImage *) "filegeneratefamily.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Generate TTC...|No Shortcut"), NULL, NULL, BVMenuGenerateTTC, 0 },
    { { (unichar_t *) N_("Expor_t..."), (GImage *) "fileexport.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, H_("Export...|No Shortcut"), NULL, NULL, BVMenuExport, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Import..."), (GImage *) "fileimport.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Import...|Ctl+Shft+I"), NULL, NULL, BVMenuImport, 0 },
    { { (unichar_t *) N_("_Revert File"), (GImage *) "filerevert.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Revert File|Ctl+Shft+R"), NULL, NULL, BVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Pr_eferences..."), (GImage *) "fileprefs.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Preferences...|No Shortcut"), NULL, NULL, MenuPrefs, 0 },
    { { (unichar_t *) N_("_X Resource Editor..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("X Resource Editor...|No Shortcut"), NULL, NULL, MenuXRes, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Quit"), (GImage *) "filequit.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'Q' }, H_("Quit|Ctl+Q"), NULL, NULL, MenuExit, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 edlist[] = {
    { { (unichar_t *) N_("_Undo"), (GImage *) "editundo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Undo|Ctl+Z"), NULL, NULL, BVUndo, MID_Undo },
    { { (unichar_t *) N_("_Redo"), (GImage *) "editredo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Redo|Ctl+Y"), NULL, NULL, BVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Cu_t"), (GImage *) "editcut.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, H_("Cut|Ctl+X"), NULL, NULL, BVCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), (GImage *) "editcopy.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Copy|Ctl+C"), NULL, NULL, BVCopy, MID_Copy },
    { { (unichar_t *) N_("C_opy Reference"), (GImage *) "editcopyref.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Copy Reference|Ctl+G"), NULL, NULL, BVCopyRef, MID_CopyRef },
    { { (unichar_t *) N_("_Paste"), (GImage *) "editpaste.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Paste|Ctl+V"), NULL, NULL, BVPaste, MID_Paste },
    { { (unichar_t *) N_("C_lear"), (GImage *) "editclear.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Clear|Delete"), NULL, NULL, BVClear, MID_Clear },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Select _All"), (GImage *) "editselect.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Select All|Ctl+A"), NULL, NULL, BVSelectAll, MID_SelAll },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Remo_ve Undoes"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Remove Undoes|No Shortcut"), NULL, NULL, BVRemoveUndoes, MID_RemoveUndoes },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("U_nlink Reference"), (GImage *) "editunlink.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Unlink Reference|Ctl+U"), NULL, NULL, BVUnlinkRef, MID_UnlinkRef },
    GMENUITEM2_EMPTY
};

static GMenuItem2 trlist[] = {
    { { (unichar_t *) N_("Flip _Horizontally"), (GImage *) "transformfliphor.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Flip Horizontally|No Shortcut"), NULL, NULL, BVMenuRotateInvoked, bvt_fliph },
    { { (unichar_t *) N_("Flip _Vertically"), (GImage *) "transformflipvert.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Flip Vertically|No Shortcut"), NULL, NULL, BVMenuRotateInvoked, bvt_flipv },
    { { (unichar_t *) NU_("_Rotate 90° CW"), (GImage *) "transformrotatecw.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Rotate 90 CW|No Shortcut"), NULL, NULL, BVMenuRotateInvoked, bvt_rotate90cw },
    { { (unichar_t *) NU_("Rotate _90° CCW"), (GImage *) "transformrotateccw.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '9' }, H_("Rotate 90 CCW|No Shortcut"), NULL, NULL, BVMenuRotateInvoked, bvt_rotate90ccw },
    { { (unichar_t *) NU_("Rotate _180°"), (GImage *) "transformrotate180.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '1' }, H_("Rotate 180|No Shortcut"), NULL, NULL, BVMenuRotateInvoked, bvt_rotate180 },
    { { (unichar_t *) N_("_Skew..."), (GImage *) "transformskew.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Skew...|No Shortcut"), NULL, NULL, BVMenuRotateInvoked, bvt_skew },
    GMENUITEM2_EMPTY
};

static GMenuItem2 ellist[] = {
    { { (unichar_t *) N_("_Font Info..."), (GImage *) "elementfontinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Font Info...|Ctl+Shft+F"), NULL, NULL, BVMenuFontInfo, 0 },
    { { (unichar_t *) N_("Glyph _Info..."), (GImage *) "elementglyphinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Glyph Info...|Ctl+I"), NULL, NULL, BVMenuGetInfo, MID_GetInfo },
    { { (unichar_t *) N_("BDF Info..."), (GImage *) "elementbdfinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("BDF Info...|No Shortcut"), NULL, NULL, BVMenuBDFInfo, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Bitm_ap Strikes Available..."), (GImage *) "elementbitmapsavail.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Bitmap Strikes Available...|Ctl+Shft+B"), NULL, NULL, BVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) N_("Regenerate _Bitmap Glyphs..."), (GImage *) "elementregenbitmaps.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Regenerate Bitmap Glyphs...|Ctl+B"), NULL, NULL, BVMenuBitmaps, MID_RegenBitmaps },
    { { (unichar_t *) N_("Remove This Glyph"), (GImage *) "elementremovebitmaps.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Remove This Glyph|No Shortcut"), NULL, NULL, BVMenuRmGlyph, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Transformations"), (GImage *) "elementtransform.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, NULL, trlist, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 pllist[] = {
    { { (unichar_t *) N_("_Tools"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Tools|No Shortcut"), NULL, NULL, BVMenuPaletteShow, MID_Tools },
    { { (unichar_t *) N_("_Layers"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Layers|No Shortcut"), NULL, NULL, BVMenuPaletteShow, MID_Layers },
    { { (unichar_t *) N_("_Shades"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'S' }, H_("Shades|No Shortcut"), NULL, NULL, BVMenuPaletteShow, MID_Shades },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Docked Palettes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'D' }, H_("Docked Palettes|No Shortcut"), NULL, NULL, BVMenuPalettesDock, MID_DockPalettes },
    GMENUITEM2_EMPTY
};

static GMenuItem2 vwlist[] = {
    { { (unichar_t *) N_("_Fit"), (GImage *) "viewfit.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Fit|Ctl+F"), NULL, NULL, BVMenuScale, MID_Fit },
    { { (unichar_t *) N_("Z_oom out"), (GImage *) "viewzoomout.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Zoom out|Alt+Ctl+-"), NULL, NULL, BVMenuScale, MID_ZoomOut },
    { { (unichar_t *) N_("Zoom _in"), (GImage *) "viewzoomin.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'i' }, H_("Zoom in|Alt+Ctl+Shft++"), NULL, NULL, BVMenuScale, MID_ZoomIn },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Next Glyph"), (GImage *) "viewnext.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("Next Glyph|Ctl+]"), NULL, NULL, BVMenuChangeChar, MID_Next },
    { { (unichar_t *) N_("_Prev Glyph"), (GImage *) "viewprev.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Prev Glyph|Ctl+["), NULL, NULL, BVMenuChangeChar, MID_Prev },
    { { (unichar_t *) N_("Next _Defined Glyph"), (GImage *) "viewnextdef.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Next Defined Glyph|Alt+Ctl+]"), NULL, NULL, BVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) N_("Prev Defined Gl_yph"), (GImage *) "viewprevdef.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Prev Defined Glyph|Alt+Ctl+["), NULL, NULL, BVMenuChangeChar, MID_PrevDef },
    { { (unichar_t *) N_("_Goto"), (GImage *) "viewgoto.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Goto|Ctl+Shft+>"), NULL, NULL, BVMenuGotoChar, 0 },
    { { (unichar_t *) N_("Find In Font _View"), (GImage *) "viewfindinfont.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Find In Font View|Ctl+Shft+<"), NULL, NULL, BVMenuFindInFontView, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Bigger Pixel Size"), (GImage *) "viewbiggersize.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Bigger Pixel Size|Ctl+Shft++"), NULL, NULL, BVMenuChangePixelSize, MID_Bigger },
    { { (unichar_t *) N_("_Smaller Pixel Size"), (GImage *) "viewsmallersize.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Smaller Pixel Size|Ctl+-"), NULL, NULL, BVMenuChangePixelSize, MID_Smaller },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Palettes"), (GImage *) "viewpalettes.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, NULL, pllist, pllistcheck, NULL, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 mtlist[] = {
    { { (unichar_t *) N_("Set _Width..."), (GImage *) "metricssetwidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Set Width...|Ctl+Shft+L"), NULL, NULL, BVMenuSetWidth, MID_SetWidth },
    { { (unichar_t *) N_("Set _Vertical Width..."), (GImage *) "metricssetvwidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Set Vertical Width...|Ctl+Shft+L"), NULL, NULL, BVMenuSetWidth, MID_SetVWidth },
    GMENUITEM2_EMPTY
};

static GMenuItem2 mblist[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, NULL, fllist, fllistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'E' }, NULL, edlist, edlistcheck, NULL, 0 },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, NULL, ellist, ellistcheck, NULL, 0 },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, NULL, vwlist, vwlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, NULL, mtlist, mtlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, NULL, wnmenu, BVWindowMenuBuild, NULL, 0 },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, NULL, helplist, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};

#define bitmap_width 16
#define bitmap_height 16
static unsigned char bitmap_bits[] = {
   0x00, 0x00, 0xfc, 0x03, 0xfc, 0x03, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x0c,
   0x30, 0x0c, 0xf0, 0x03, 0xf0, 0x03, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x0c,
   0x30, 0x0c, 0xfc, 0x03, 0xfc, 0x03, 0x00, 0x00};

static int bitmapview_ready = false;

static void BitmapViewFinish() {
  if ( !bitmapview_ready ) return;
  bitmapview_ready = 0;
  mb2FreeGetText(mblist);
}

void BitmapViewFinishNonStatic() {
  BitmapViewFinish();
}

static void BitmapViewInit(void) {
    // static int done = false; // superseded by bitmapview_ready.
    int i;

    if ( bitmapview_ready )
return;
    bitmapview_ready = true;

    mb2DoGetText(mblist);
    for ( i=0; BVFlipNames[i]!=NULL ; ++i )
	BVFlipNames[i] = S_(BVFlipNames[i]);
    atexit(&BitmapViewFinishNonStatic);
}

BitmapView *BitmapViewCreate(BDFChar *bc, BDFFont *bdf, FontView *fv, int enc) {
    BitmapView *bv = calloc(1,sizeof(BitmapView));
    GRect pos, zoom, size;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GRect gsize;
    int sbsize;
    char buf[300];
    static GWindow icon = NULL;
    GTextInfo ti;
    FontRequest rq;
    int as, ds, ld;
    static char *infofamily = NULL;

    BitmapViewInit();

    BVShows.lastpixelsize = bdf->pixelsize;

    if ( icon==NULL )
	icon = GDrawCreateBitmap(NULL,bitmap_width,bitmap_height,bitmap_bits);

    bv->bc = bc;
    bv->scale = 1;
    bv->xoff = bv->yoff = 20;
    bv->next = bc->views;
    bc->views = bv;
    bv->fv = fv;
    bv->enc = enc;
    bv->map_of_enc = fv->b.map;
    bv->bdf = bdf;
    bv->color = 255;
    bv->shades_hidden = bdf->clut==NULL;

    bv->showfore = BVShows.showfore;
    bv->showoutline = BVShows.showoutline;
    bv->showgrid = BVShows.showgrid;
    bv->scscale = ((real) (bdf->pixelsize))/(bdf->sf->ascent+bdf->sf->descent);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_utf8_ititle;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    wattrs.utf8_icon_title = BVMakeTitles(bv,bc,buf);
    wattrs.utf8_window_title = buf;
    wattrs.icon = icon;
    if ( wattrs.icon )
	wattrs.mask |= wam_icon;
    pos.x = 8+9*16+10; pos.width=bv_width; pos.height = bv_height;
    DefaultY(&pos);

    bv->gw = gw = GDrawCreateTopWindow(NULL,&pos,bv_e_h,bv,&wattrs);
    free( (unichar_t *) wattrs.icon_title );

    GDrawGetSize(GDrawGetRoot(screen_display),&zoom);
    zoom.x = BVPalettesWidth(); zoom.width -= zoom.x-10;
    zoom.height -= 30;			/* Room for title bar & such */
    GDrawSetZoom(gw,&zoom,-1);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = BVMenuContextualHelp;
    gd.u.menu2 = mblist;
    bv->mb = GMenu2BarCreate( gw, &gd, NULL);
    GGadgetGetSize(bv->mb,&gsize);
    bv->mbh = gsize.height;
    bv->infoh = GDrawPointsToPixels(gw,36);

    gd.pos.y = bv->mbh+bv->infoh;
    gd.pos.width = sbsize = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.height = pos.height-bv->mbh-bv->infoh - sbsize;
    gd.pos.x = pos.width-sbsize;
    gd.u.sbinit = NULL;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    bv->vsb = GScrollBarCreate(gw,&gd,bv);

    gd.pos.y = pos.height-sbsize; gd.pos.height = sbsize;
    gd.pos.width = pos.width - sbsize;
    gd.pos.x = 0;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    bv->hsb = GScrollBarCreate(gw,&gd,bv);

    memset(&gd, '\0', sizeof(gd));
    memset(&ti, '\0', sizeof(ti));
    gd.pos.x = pos.width - GDrawPointsToPixels(gw,111);
    gd.pos.y = bv->mbh + GDrawPointsToPixels(gw,6);
    /*gd.pos.width = GDrawPointsToPixels(gw,106);*/
    gd.label = &ti;
    ti.text = (unichar_t *) _("Recalculate Bitmaps");
    ti.text_is_1byte = true;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    if ( fv->b.sf->onlybitmaps )
	gd.flags = gg_pos_in_pixels;
    gd.handle_controlevent = BVRecalc;
    bv->recalc = GButtonCreate(gw,&gd,bv);
    GGadgetGetSize(bv->recalc,&size);
    GGadgetMove(bv->recalc,pos.width - size.width - GDrawPointsToPixels(gw,6),size.y);

    pos.y = bv->mbh+bv->infoh; pos.height -= bv->mbh + sbsize + bv->infoh;
    pos.x = 0; pos.width -= sbsize;
    wattrs.mask = wam_events|wam_cursor|wam_backcol;
    wattrs.background_color = view_bgcol;
    wattrs.event_masks = -1;
    wattrs.cursor = ( bc->refs == NULL ) ? ct_pencil : ct_pointer;
    bv->v = GWidgetCreateSubWindow(gw,&pos,v_e_h,bv,&wattrs);

    bv->height = pos.height; bv->width = pos.width;
    bv->b1_tool = ( bc->refs == NULL ) ? bvt_pencil : bvt_pointer; bv->cb1_tool = bvt_pointer;
    bv->b2_tool = bvt_magnify; bv->cb2_tool = bvt_shift;
    bv->s1_tool = bv->s2_tool = bv->er_tool = bvt_pointer;
    bv->showing_tool = ( bc->refs == NULL ) ? bvt_pencil : bvt_pointer;
    bv->pressed_tool = bv->pressed_display = bv->active_tool = bvt_none;

    /*GWidgetHidePalettes();*/
    /*bv->tools = BVMakeTools(bv);*/
    /*bv->layers = BVMakeLayers(bv);*/

    if ( infofamily==NULL ) {	/* Yes, let's use the same resource name */
	infofamily = copy(GResourceFindString("CharView.InfoFamily"));
	/* FontConfig doesn't have access to all the X11 bitmap fonts */
	/*  so the font I used to use isn't found, and a huge monster is */
	/*  inserted instead */
	if ( infofamily==NULL )
	    infofamily = SANS_UI_FAMILIES;
    }

    memset(&rq,0,sizeof(rq));
    rq.utf8_family_name = infofamily;
    rq.point_size = -7;
    rq.weight = 400;
    bv->small = GDrawInstanciateFont(gw,&rq);
    GDrawWindowFontMetrics(gw,bv->small,&as,&ds,&ld);
    bv->sfh = as+ds; bv->sas = as;

    BVFit(bv);
    GDrawSetVisible(bv->v,true);
    GDrawSetVisible(gw,true);
return( bv );
}

BitmapView *BitmapViewCreatePick(int enc, FontView *fv) {
    BDFFont *bdf;
    SplineFont *sf;
    EncMap *map;

    sf = fv->b.cidmaster ? fv->b.cidmaster : fv->b.sf;
    map = fv->b.map;

    if ( fv->show!=fv->filled )
	bdf = fv->show;
    else
	for ( bdf = sf->bitmaps; bdf!=NULL && bdf->pixelsize!=BVShows.lastpixelsize; bdf = bdf->next );
    if ( bdf==NULL )
	bdf = sf->bitmaps;

return( BitmapViewCreate(BDFMakeChar(bdf,map,enc),bdf,fv,enc));
}

void BitmapViewFree(BitmapView *bv) {
    free(bv);
}

static void BC_RefreshAll(BDFChar *bc) {
    BitmapView *bv;

    for ( bv = bc->views; bv!=NULL; bv = bv->next )
	GDrawRequestExpose(bv->v,NULL,false);
}

static void BC_DestroyAll(BDFChar *bc) {
    BitmapView *bv, *next;

    for ( bv = bc->views; bv!=NULL; bv=next ) {
	next = bv->next;
	GDrawDestroyWindow(bv->gw);
    }
}

struct bc_interface gdraw_bc_interface = {
    BC_CharChangedUpdate,
    BC_RefreshAll,
    BC_DestroyAll
};
