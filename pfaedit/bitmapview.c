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
#include "gkeysym.h"
#include "utype.h"
#include "ustring.h"
#include <math.h>

extern int _GScrollBar_Width;
struct bvshows BVShows = { 1, 1, 1, 0 };

#define RPT_BASE	3		/* Place to draw the pointer icon */
#define RPT_DATA	24		/* x,y text after above */

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
    if ( top<bottom ) GDrawIError("Bottom bigger than top!");
    if ( right<left ) GDrawIError("Left bigger than right!");
    top -= bottom;
    right -= left;
    if ( top==0 ) top = bv->bdf->pixelsize;
    if ( right==0 ) right = bv->bdf->pixelsize;
    wsc = (8*bv->width) / (10*right);
    hsc = (8*bv->height) / (10*top);
    if ( wsc<hsc ) hsc = wsc;
    if ( hsc<=0 ) hsc = 1;
    if ( hsc>32 ) hsc = 32;

    bv->scale = hsc;

    bv->xoff = left+(bv->width-right*bv->scale)/2;
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

void BCCharChangedUpdate(BDFChar *bc,FontView *fv) {
    BDFFont *bdf;
    BitmapView *bv;
    int waschanged = bc->changed;

    bc->changed = true;
    for ( bv = bc->views; bv!=NULL; bv=bv->next ) {
	GDrawRequestExpose(bv->v, NULL, false );
	BVRefreshImage(bv);
	if ( fv==NULL )
	    fv = bv->fv;
    }
    if ( fv!=NULL ) {
	fv->sf->changed = true;
	if ( fv->show!=fv->filled ) {
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL && bdf->chars[bc->enc]!=bc; bdf=bdf->next );
	    if ( bdf!=NULL ) {
		FVRefreshChar(fv,bdf,bc->enc);
		if ( fv->sf->onlybitmaps && !waschanged )
		    FVToggleCharChanged(fv,fv->sf->chars[bc->enc]);
	    }
	}
    }
}

BDFChar *BDFMakeChar(BDFFont *bdf,int i) {
    SplineChar *sc = SFMakeChar(bdf->sf,i);
    BDFChar *bc;

    if ( (bc = bdf->chars[i])==NULL ) {
	bc = bdf->chars[i] = SplineCharRasterize(sc,bdf->pixelsize);
	bc->enc = i;
    }
return( bc );
}

void BVChangeBC(BitmapView *bv, BDFChar *bc, int fitit ) {
    char buffer[210];
    unichar_t *title;
    unichar_t ubuf[300];
    SplineChar *sc;
    BDFFont *bdf = bv->bdf;

    BVUnlinkView(bv);
    bv->bc = bc;
    bv->next = bc->views;
    bc->views = bv;

    if ( fitit )
	BVFit(bv);
    else
	BVNewScale(bv);
    BVRefreshImage(bv);

    sc = bdf->sf->chars[bc->enc];
    sprintf( buffer, "%.90s at %d from %.90s ", sc->name, bdf->pixelsize,
	    sc->parent->fontname );
    title = uc_copy(buffer);
    u_strcpy(ubuf,title);
    if ( sc->unicodeenc!=-1 && UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]!=NULL )
	u_strcat(ubuf, UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]);
    GDrawSetWindowTitles(bv->gw,ubuf,title);
    free(title);
}

static void BVChangeChar(BitmapView *bv, int i, int fitit ) {
    BDFChar *bc;
    BDFFont *bdf = bv->bdf;

    if ( i<0 || i>=bdf->charcnt )
return;
    bc = BDFMakeChar(bdf,i);

    if ( bc==NULL || bv->bc == bc )
return;
    BVChangeBC(bv,bc,fitit);
}

static void BVDoClear(BitmapView *bv);

void BVChar(BitmapView *bv, GEvent *event ) {

    BVToolsSetCursor(bv,TrueCharState(event));
    if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->u.chr.keysym == GK_BackSpace ) {
	/* Menu does delete */
	BVDoClear(bv);
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
	BCPreserveState(bv->bc);
	if ( bv->bc->selection==NULL ) {
	    bv->bc->xmin += xoff;  bv->bc->xmax += xoff;
	    bv->bc->ymin += yoff;  bv->bc->ymax += yoff;
	} else {
	    bv->bc->selection->xmin += xoff;  bv->bc->selection->xmax += xoff;
	    bv->bc->selection->ymin += yoff;  bv->bc->selection->ymax += yoff;
	}
	BCCharChangedUpdate(bv->bc,bv->fv);
    } else if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->type == et_char &&
	    event->u.chr.chars[0]!='\0' && event->u.chr.chars[1]=='\0' ) {
	SplineFont *sf = bv->bc->sc->parent;
	int i;
	for ( i=0; i<sf->charcnt; ++i )
	    if ( sf->chars[i]!=NULL )
		if ( sf->chars[i]->unicodeenc==event->u.chr.chars[0] )
		    BVChangeChar(bv,i,false);
    }
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

static void BVDrawSelection(BitmapView *bv,void *pixmap) {
    GRect pixel, rect;
    BDFFloat *sel = bv->bc->selection;
    Color bg = GDrawGetDefaultBackground(NULL);
    int i,j;

    pixel.width = pixel.height = bv->scale+1;
    for ( i=sel->ymax-sel->ymin; i>=0; --i ) {
	for ( j=0; j<=sel->xmax-sel->xmin; ++j ) {
	    pixel.x = bv->xoff + (sel->xmin+j)*bv->scale;
	    pixel.y = bv->height-bv->yoff-(sel->ymax-i+1)*bv->scale;
	    if ( sel->bitmap[i*sel->bytes_per_line+(j>>3)] & (1<<(7-(j&7))) ) {
		GDrawFillRect(pixmap,&pixel,0x808080);
	    } else {
		GDrawFillRect(pixmap,&pixel,bg);
	    }
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

static void BVExpose(BitmapView *bv, GWindow pixmap, GEvent *event ) {
    CharView cvtemp;
    GRect old;
    DRect clip;
    int i,j;
    GRect pixel;
    BDFChar *bc = bv->bc;
    RefChar *refs;

    GDrawPushClip(pixmap,&event->u.expose.rect,&old);
    if ( bv->showfore ) {
	/* fore ground is a misnomer. it's what we're interested in but we */
	/*  actually need to draw it first, otherwise it obscures everything */
	pixel.width = pixel.height = bv->scale+1;
	for ( i=bc->ymax-bc->ymin; i>=0; --i ) {
	    for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		if ( bc->bitmap[i*bc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))) ) {
		    pixel.x = bv->xoff + (bc->xmin+j)*bv->scale;
		    pixel.y = bv->height-bv->yoff-(bc->ymax-i+1)*bv->scale;
		    GDrawFillRect(pixmap,&pixel,0x808080);
		}
	    }
	}
	if ( bv->active_tool!=bvt_none ) {
	    /* This does nothing for many tools, but for lines, rects and circles */
	    /*  it draws temporary points */
	    BCGeneralFunction(bv,BVDrawTempPoint,pixmap);
	}
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
	GDrawDrawLine(pixmap,bv->xoff+bv->bc->width*bv->scale,0, bv->xoff+bv->bc->width*bv->scale,bv->height,0x000000);
    }
    if ( bv->showoutline ) {
	memset(&cvtemp,'\0',sizeof(cvtemp));
	cvtemp.v = bv->v;
	cvtemp.width = bv->width;
	cvtemp.height = bv->height;
	cvtemp.scale = bv->scscale*bv->scale;
	cvtemp.xoff = bv->xoff/* *bv->scscale*/;
	cvtemp.yoff = bv->yoff/* *bv->scscale*/;
	cvtemp.sc = bv->bc->sc;
	cvtemp.drawmode = dm_fore;

	clip.width = event->u.expose.rect.width/cvtemp.scale;
	clip.height = event->u.expose.rect.height/cvtemp.scale;
	clip.x = (event->u.expose.rect.x-cvtemp.xoff)/cvtemp.scale;
	clip.y = (cvtemp.height-event->u.expose.rect.y-event->u.expose.rect.height-cvtemp.yoff)/cvtemp.scale;
	CVDrawSplineSet(&cvtemp,pixmap,cvtemp.sc->splines,0x004400,false,&clip);
	for ( refs = cvtemp.sc->refs; refs!=NULL; refs = refs->next )
	    CVDrawSplineSet(&cvtemp,pixmap,refs->splines,0x004400,false,&clip);
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
    unichar_t ubuffer[50];
    int ybase = bv->mbh+10+bv->sas;

    GDrawSetFont(pixmap,bv->small);
    r.x = bv->infoh+RPT_DATA; r.width = 39;
    r.y = bv->mbh; r.height = bv->infoh-1;
    GDrawFillRect(pixmap,&r,bg);

    sprintf(buffer,"%d,%d", bv->info_x, bv->info_y );
    buffer[11] = '\0';
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,bv->infoh+RPT_DATA,ybase,ubuffer,-1,NULL,0);

    if ( bv->active_tool!=cvt_none ) {
	sprintf(buffer,"%d,%d", bv->info_x-bv->pressed_x, bv->info_y-bv->pressed_y );
	buffer[11] = '\0';
	uc_strcpy(ubuffer,buffer);
	GDrawDrawText(pixmap,bv->infoh+RPT_DATA,ybase+bv->sfh+10,ubuffer,-1,NULL,0);
    }
}

static void BVMainExpose(BitmapView *bv, GWindow pixmap, GEvent *event ) {
    GRect old, temp, box, old2;
    GImage gi;
    struct _GImage base;
    GClut clut;
    BDFChar *bdfc = bv->bc;

    temp = event->u.expose.rect;
    if ( temp.y+temp.height < bv->mbh )
return;
    if ( temp.y <bv->mbh ) {
	temp.height -= (bv->mbh-temp.y);
	temp.y = bv->mbh;
    }
    GDrawPushClip(pixmap,&temp,&old);

    if ( event->u.expose.rect.x<6+bdfc->xmax-bdfc->xmin ) {
	box.x = 0; box.width = bv->infoh;
	box.y = bv->mbh; box.height = bv->infoh;
	GDrawPushClip(pixmap,&box,&old2);

	memset(&gi,'\0',sizeof(gi));
	memset(&base,'\0',sizeof(base));
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	base.image_type = it_mono;
	base.clut = &clut;
	clut.clut_len = 2;
	clut.clut[0] = 0xffffff;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	GDrawDrawImage(pixmap,&gi,NULL, 5,bv->mbh+(bv->infoh-base.height)/2);

	GDrawPopClip(pixmap,&old2);

	GDrawDrawImage(pixmap,&GIcon_rightpointer,NULL,bv->infoh+RPT_BASE,bv->mbh+8);
	GDrawDrawImage(pixmap,&GIcon_press2ptr,NULL,bv->infoh+RPT_BASE,bv->mbh+18+bv->sfh);
	BVInfoDrawText(bv,pixmap );
    }
    GDrawDrawLine(pixmap,0,bv->mbh+bv->infoh-1,bv->width+300,bv->mbh+bv->infoh-1,0);

    GDrawPopClip(pixmap,&old);
}

static void BVShowInfo(BitmapView *bv) {
    BVInfoDrawText(bv,bv->gw );
}

static void BVResize(BitmapView *bv, GEvent *event ) {
    int sbsize = GDrawPointsToPixels(bv->gw,_GScrollBar_Width);
    int newwidth = event->u.resize.size.width-sbsize,
	newheight = event->u.resize.size.height-sbsize - bv->mbh-bv->infoh;

    if ( newwidth == bv->width && newheight == bv->height )
return;

    /* MenuBar takes care of itself */
    GDrawResize(bv->v,newwidth,newheight);
    GGadgetMove(bv->vsb,newwidth, bv->mbh+bv->infoh);
    GGadgetResize(bv->vsb,sbsize,newheight);
    GGadgetMove(bv->hsb,0,event->u.resize.size.height-sbsize);
    GGadgetResize(bv->hsb,newwidth,sbsize);
    bv->width = newwidth; bv->height = newheight;
    BVFit(bv);
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

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	bv = GDrawGetUserData(GGadgetGetWindow(g));
	BCPreserveState(bv->bc);
	BCFlattenFloat(bv->bc);
	bdfc = SplineCharRasterize(bv->bc->sc,bv->bdf->pixelsize);
	free(bv->bc->bitmap);
	bv->bc->bitmap = bdfc->bitmap; bdfc->bitmap = NULL;
	bv->bc->width = bdfc->width;
	bv->bc->xmin = bdfc->xmin;
	bv->bc->xmax = bdfc->xmax;
	bv->bc->ymin = bdfc->ymin;
	bv->bc->ymax = bdfc->ymax;
	bv->bc->bytes_per_line = bdfc->bytes_per_line;
	BDFCharFree(bdfc);
	BCCharChangedUpdate(bv->bc,bv->fv);
    }
return( true );
}

static void BVSetWidth(BitmapView *bv, int x) {
    int tot, cnt;
    BDFFont *bdf;
    BDFChar *bc = bv->bc;

    if ( bv->fv->sf->onlybitmaps ) {
	bc->width = x;
	tot=0; cnt=0;
	for ( bdf = bv->fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->chars[bc->enc]) {
		tot += bdf->chars[bc->enc]->width*1000/(bdf->ascent+bdf->descent);
		++cnt;
	    }
	if ( cnt!=0 )
	    bv->fv->sf->chars[bc->enc]->width = tot/cnt;
	BCCharChangedUpdate(bc,bv->fv);
    }
}

static void BVMouseDown(BitmapView *bv, GEvent *event) {
    int x = floor( (event->u.mouse.x-bv->xoff)/ (double) bv->scale);
    int y = floor( (bv->height-event->u.mouse.y-bv->yoff)/ (double) bv->scale);
    int ny;
    BDFChar *bc = bv->bc;
    BDFFloat *sel;

    if ( event->u.mouse.button==3 ) {
	BVToolsPopup(bv,event);
return;
    }
    BVToolsSetCursor(bv,TrueCharState(event));
    bv->active_tool = bv->showing_tool;
    bv->pressed_x = x; bv->pressed_y = y;
    bv->info_x = x; bv->info_y = y;
    bv->event_x = event->u.mouse.x; bv->event_y = event->u.mouse.y;
    bv->recentchange = false;
    switch ( bv->active_tool ) {
      case bvt_pencil: case bvt_line:
      case bvt_rect: case bvt_filledrect:
	ny = bc->ymax-y;
	bv->clearing = false;
	if ( x>=bc->xmin && x<=bc->xmax && ny>=0 && ny<=bc->ymax-bc->ymin ) {
	    int nx = x-bc->xmin;
	    if ( bc->bitmap[ny*bc->bytes_per_line + (nx>>3)] &
		    (1<<(7-(nx&7))) )
		bv->clearing = true;
	}
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	if ( bv->active_tool == bvt_pencil )
	    BCSetPoint(bc,x,y,bv->clearing);
	BCCharChangedUpdate(bc,bv->fv);
      break;
      case bvt_elipse: case bvt_filledelipse:
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	BCCharChangedUpdate(bc,bv->fv);
      break;
      case bvt_pointer:
	if ( (sel = bc->selection)!=NULL ) {
	    if ( x<sel->xmin || x>sel->xmax || y<sel->ymin || y>sel->ymax )
		BCFlattenFloat(bc);
	    else {
		GDrawSetCursor(bv->v,ct_shift);
		/* otherwise we'll move the selection */
	    }
	}
	BCCharUpdate(bc);
      break;
      case bvt_setwidth:
	BVSetWidth(bv,x);
      break;
    }
}

static void BVMouseMove(BitmapView *bv, GEvent *event) {
    int x = floor( (event->u.mouse.x-bv->xoff)/ (double) bv->scale);
    int y = floor( (bv->height-event->u.mouse.y-bv->yoff)/ (double) bv->scale);
    int newx, newy;
    int fh = bv->bdf->ascent+bv->bdf->descent;
    BDFChar *bc = bv->bc;

    bv->info_x = x; bv->info_y = y;
    BVShowInfo(bv);
    if ( bv->active_tool==bvt_none )
return;			/* Not pressed */
    switch ( bv->active_tool ) {
      case bvt_pencil:
	BCSetPoint(bc,x,y,bv->clearing);
	BCCharChangedUpdate(bc,bv->fv);
      break;
      case bvt_line: case bvt_rect: case bvt_filledrect:
      case bvt_elipse: case bvt_filledelipse:
	BCCharChangedUpdate(bc,bv->fv);
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
	    BCCharChangedUpdate(bc,bv->fv);
	    bv->pressed_x = x; bv->pressed_y = y;
	}
      break;
      case bvt_pointer:
	if ( bc->selection!=NULL ) {
	    if ( x!=bv->pressed_x || y!=bv->pressed_y ) {
		if ( !bv->recentchange ) {
		    BCPreserveState(bc);
		    bv->recentchange = true;
		}
		bc->selection->xmin += x-bv->pressed_x;
		bc->selection->xmax += x-bv->pressed_x;
		bc->selection->ymin += y-bv->pressed_y;
		bc->selection->ymax += y-bv->pressed_y;
		BCCharChangedUpdate(bc,bv->fv);
		bv->pressed_x = x; bv->pressed_y = y;
	    }
	} else {
	    GDrawRequestExpose(bv->v,NULL,false);
	}
      break;
      case bvt_setwidth:
	BVSetWidth(bv,x);
      break;
    }
}

static void BVSetPoint(BitmapView *bv, int x, int y, void *junk) {
    BCSetPoint(bv->bc,x,y,bv->clearing);
}

static void BVMagnify(BitmapView *bv, int midx, int midy, int bigger);

static void BVMouseUp(BitmapView *bv, GEvent *event) {
    int x = floor( (event->u.mouse.x-bv->xoff)/ (double) bv->scale);
    int y = floor( (bv->height-event->u.mouse.y-bv->yoff)/ (double) bv->scale);

    BVMouseMove(bv,event);
    switch ( bv->active_tool ) {
      case bvt_magnify: case bvt_minify:
	BVMagnify(bv,x,y,bv->active_tool==bvt_magnify?1:-1);
      break;
      case bvt_line: case bvt_rect: case bvt_filledrect:
      case bvt_elipse: case bvt_filledelipse:
	BCGeneralFunction(bv,BVSetPoint,NULL);
	bv->active_tool = bvt_none;
	BCCharChangedUpdate(bv->bc,bv->fv);
      break;
      case bvt_pointer:
	if ( bv->bc->selection!=NULL ) {
	    /* we've been moving it */
	    GDrawSetCursor(bv->v,ct_mypointer);
	    if ( !bv->recentchange ) {	/* Oh, we just clicked in it, get rid of it */
		BCFlattenFloat(bv->bc);
		BCCharChangedUpdate(bv->bc,bv->fv);
	    }
	} else {
	    int dx,dy;
	    if ( (dx = event->u.mouse.x-bv->event_x)<0 ) dx = -dx;
	    if ( (dy = event->u.mouse.y-bv->event_y)<0 ) dy = -dy;
	    if ( dx+dy>4 ) {
		/* we've just dragged out a new one */
		BDFFloatCreate(bv->bc,bv->pressed_x,bv->info_x,bv->pressed_y,bv->info_y,true);
	    }
	    bv->active_tool = bvt_none;
	    BCCharChangedUpdate(bv->bc,bv->fv);
	}
      break;
      case bvt_setwidth:
	BVSetWidth(bv,x);
      break;
    }
    bv->active_tool = bvt_none;
    BVToolsSetCursor(bv,TrueCharState(event));
}

static int v_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	BVExpose(bv,gw,event);
      break;
      case et_crossing:
	BVToolsSetCursor(bv,event->u.mouse.state);
      break;
      case et_mousedown:
	BVMouseDown(bv,event);
      break;
      case et_mousemove:
	BVMouseMove(bv,event);
      break;
      case et_mouseup:
	BVMouseUp(bv,event);
	BVToolsSetCursor(bv,TrueCharState(event));
      break;
      case et_char:
	BVChar(bv,event);
      break;
      case et_charup:
	BVToolsSetCursor(bv,TrueCharState(event));
      break;
      case et_timer:
      break;
    }
return( true );
}

static int bv_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	BVMainExpose(bv,gw,event);
      break;
      case et_char:
	BVChar(bv,event);
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
	BitmapViewFree(bv);
      break;
      case et_close:
	GDrawDestroyWindow(gw);
      break;
      case et_mouseup: case et_mousedown:
	GGadgetEndPopup();
      break;
      case et_mousemove:
	SCPreparePopup(bv->gw,bv->bc->sc);
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
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_SelAll	2106
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_GetInfo	2203
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Tools	2501
#define MID_Layers	2502
#define MID_Revert	2702
#define MID_Recent	2703

static void BVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    GDrawDestroyWindow(gw);
}
	
static void BVMenuOpenOutline(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    CharViewCreate(bv->bc->sc,bv->fv);
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
    _FVMenuGenerate(bv->fv);
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
    FVDelay(bv->fv,FVRevert);		/* The revert command can potentially */
			    /* destroy our window (if the char weren't in the */
			    /* old font). If that happens before the menu finishes */
			    /* we get a crash. So delay till after the menu completes */
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Revert:
	    mi->ti.disabled = bv->fv->sf->origname==NULL;
	  break;
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static void BVMagnify(BitmapView *bv, int midx, int midy, int bigger) {

    if ( bigger>0 ) {
	bv->scale *= 2;
	if ( bv->scale > 32 ) bv->scale = 32;
    } else {
	bv->scale /= 2;
	if ( bv->scale < 1 ) bv->scale = 1;
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
	double midx = (bv->width/2-bv->xoff)/bv->scale;
	double midy = (bv->height/2-bv->yoff)/bv->scale;
	BVMagnify(bv,midx,midy,mi->mid==MID_ZoomOut?-1:1);
    }
}

static void BVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    int pos = -1;

    if ( mi->mid == MID_Next ) {
	pos = bv->bc->enc+1;
    } else {
	pos = bv->bc->enc-1;
    }
    if ( pos>=0 && pos<bv->fv->sf->charcnt )
	BVChangeChar(bv,pos,false);
}

static void BVMenuChangePixelSize(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BDFFont *best=NULL, *bdf;
    int mysize = bv->bdf->pixelsize;

    if ( mi->mid == MID_Bigger ) {
	for ( bdf=bv->fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->pixelsize>mysize && ( best==NULL || bdf->pixelsize<best->pixelsize))
		best = bdf;
	}
    } else {
	for ( bdf=bv->fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->pixelsize<mysize && ( best==NULL || bdf->pixelsize>best->pixelsize))
		best = bdf;
	}
    }
    if ( best!=NULL && bv->bdf!=best ) {
	bv->bdf = best;
	bv->scscale = ((double) (best->pixelsize))/(best->sf->ascent+best->sf->descent);
	BVChangeChar(bv,bv->bc->enc,true);
	BVShows.lastpixelsize = best->pixelsize;
    }
}

static void BVMenuGotoChar(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    int pos = GotoChar(bv->fv->sf);

    if ( pos!=-1 )
	BVChangeChar(bv,pos,false);
}

static void BVMenuPaletteShow(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    GWindow palette = mi->mid==MID_Tools ? bv->tools : bv->layers;

    if ( palette==NULL )
return;
    GDrawSetVisible(palette,!GDrawIsVisible(palette));
}

static void BVUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    if ( bv->bc->undoes==NULL )
return;
    BCDoUndo(bv->bc,bv->fv);
}

static void BVRedo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    if ( bv->bc->redoes==NULL )
return;
    BCDoRedo(bv->bc,bv->fv);
}

static void BVCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BCCopySelected(bv->bc,bv->bdf->pixelsize);
}

static void BVDoClear(BitmapView *bv) {
    if ( bv->bc->selection!=NULL ) {
	BCPreserveState(bv->bc);
	BDFFloatFree(bv->bc->selection);
	bv->bc->selection = NULL;
	BCCharChangedUpdate(bv->bc,bv->fv);
    }
}

static void BVClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BVDoClear(bv);
}

static void BVPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    if ( CopyContainsBitmap())
	PasteToBC(bv->bc,bv->bdf->pixelsize,bv->fv);
}

static void BVCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    if ( bv->bc->selection==NULL )
return;
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
    FontMenuFontInfo(bv->fv->sf,bv->fv);
}

static void BVMenuPrivateInfo(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    SFPrivateInfo(bv->fv->sf);
}

static void BVMenuGetInfo(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    SCGetInfo(bv->bc->sc,false);
}

static void BVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BitmapDlg(bv->fv,bv->bc->sc,mi->mid==MID_AvailBitmaps );
}

void BVMenuRotateInvoked(GWindow gw,struct gmenuitem *mi,GEvent *g) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    BVRotateBitmap(bv,mi->mid);
}

static void edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Cut: /*case MID_Copy:*/ case MID_Clear:
	    /* If nothing is selected, copy copies everything */
	    mi->ti.disabled = bv->bc->selection==NULL;
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
	}
    }
}

static void pllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Tools:
	    mi->ti.checked = bv->tools!=NULL && GDrawIsVisible(bv->tools);
	  break;
	  case MID_Layers:
	    mi->ti.checked = bv->tools!=NULL && GDrawIsVisible(bv->layers);
	  break;
	}
    }
}

static void vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);
    int mysize = bv->bdf->pixelsize;
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
	    for ( bdf = bv->fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize<=mysize; bdf=bdf->next );
	    mi->ti.disabled = bdf==NULL;
	  break;
	  case MID_Smaller:
	    for ( bdf = bv->fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize>=mysize; bdf=bdf->next );
	    mi->ti.disabled = bdf==NULL;
	  break;
	}
    }
}

static unichar_t file[] = { 'F', 'i', 'l', 'e', '\0' };
static unichar_t edit[] = { 'E', 'd', 'i', 't', '\0' };
static unichar_t element[] = { 'E', 'l', 'e', 'm', 'e', 'n', 't', '\0' };
static unichar_t view[] = { 'V', 'i', 'e', 'w', '\0' };
static unichar_t window[] = { 'W', 'i', 'n', 'd', 'o', 'w', '\0' };
static unichar_t new[] = { 'N', 'e', 'w', '\0' };
static unichar_t open[] = { 'O', 'p', 'e', 'n', '\0' };
static unichar_t recent[] = { 'R', 'e', 'c', 'e', 'n', 't',  '\0' };
static unichar_t openoutline[] = { 'O', 'p', 'e', 'n', ' ', 'O', 'u', 't', 'l', 'i', 'n', 'e', '\0' };
static unichar_t openbitmap[] = { 'O', 'p', 'e', 'n', ' ', 'B', 'i', 't', 'm', 'a', 'p', '\0' };
static unichar_t openmetrics[] = { 'O', 'p', 'e', 'n', ' ', 'M', 'e', 't', 'r', 'i', 'c', 's', '\0' };
static unichar_t revert[] = { 'R', 'e', 'v', 'e','r','t',' ', 'F','i','l','e','\0' };
static unichar_t save[] = { 'S', 'a', 'v', 'e',  '\0' };
static unichar_t saveas[] = { 'S', 'a', 'v', 'e', ' ', 'a', 's', '.', '.', '.', '\0' };
static unichar_t generate[] = { 'G', 'e', 'n', 'e', 'r', 'a', 't', 'e', ' ', 'F','o','n','t','s', '.', '.', '.', '\0' };
static unichar_t export[] = { 'E','x','p','o','r', 't', '.', '.', '.',  '\0' };
static unichar_t import[] = { 'I', 'm', 'p', 'o', 'r', 't',  '.', '.', '.', '\0' };
static unichar_t close[] = { 'C', 'l', 'o', 's', 'e', '\0' };
static unichar_t prefs[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e', 's', '.', '.', '.', '\0' };
static unichar_t quit[] = { 'Q', 'u', 'i', 't', '\0' };
static unichar_t fit[] = { 'F', 'i', 't', '\0' };
static unichar_t zoomin[] = { 'Z', 'o', 'o', 'm', ' ', 'i', 'n', '\0' };
static unichar_t zoomout[] = { 'Z', 'o', 'o', 'm', ' ', 'o', 'u', 't', '\0' };
static unichar_t bigger[] = { 'B', 'i', 'g', 'g', 'e', 'r', ' ', 'P', 'i', 'x', 'e', 'l', ' ', 'S', 'i', 'z', 'e', '\0' };
static unichar_t smaller[] = { 'S', 'm', 'a', 'l', 'l', 'e', 'r', ' ', 'P', 'i', 'x', 'e', 'l', ' ', 'S', 'i', 'z', 'e', '\0' };
static unichar_t next[] = { 'N', 'e', 'x', 't', ' ', 'C', 'h', 'a', 'r', '\0' };
static unichar_t prev[] = { 'P', 'r', 'e', 'v', ' ', 'C', 'h', 'a', 'r', '\0' };
static unichar_t _goto[] = { 'G', 'o', 't', 'o',  '\0' };
static unichar_t undo[] = { 'U', 'n', 'd', 'o',  '\0' };
static unichar_t redo[] = { 'R', 'e', 'd', 'o',  '\0' };
static unichar_t cut[] = { 'C', 'u', 't',  '\0' };
static unichar_t _copy[] = { 'C', 'o', 'p', 'y',  '\0' };
static unichar_t paste[] = { 'P', 'a', 's', 't', 'e',  '\0' };
static unichar_t clear[] = { 'C', 'l', 'e', 'a', 'r',  '\0' };
static unichar_t selectall[] = { 'S', 'e', 'l', 'e', 'c', 't', ' ', 'A', 'l', 'l', '\0' };
static unichar_t fontinfo[] = { 'F','o','n','t',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t privateinfo[] = { 'P','r','i','v','a','t','e',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t getinfo[] = { 'C','h','a','r',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t bitmapsavail[] = { 'B','i','t','m','a','p','s',' ','A','v','a','i','l','a','b','l','e','.', '.', '.',  '\0' };
static unichar_t regenbitmaps[] = { 'R','e','g','e','n','e','r','a','t','e', ' ', 'B','i','t','m','a','p','s','.', '.', '.',  '\0' };
static unichar_t palettes[] = { 'P','a','l','e','t', 't', 'e', 's',  '\0' };
static unichar_t tools[] = { 'T','o','o','l','s',  '\0' };
static unichar_t layers[] = { 'L','a','y','e', 'r','s',  '\0' };
static unichar_t fliph[] = { 'F','l','i','p', ' ','H','o','r','i','z', 'o', 'n', 't', 'a', 'l', 'l', 'y',  '\0' };
static unichar_t flipv[] = { 'F','l','i','p', ' ','V','e','r','t','i', 'c', 'a', 'l', 'l', 'y',  '\0' };
static unichar_t rotate90cw[] = { 'R','o','t','a','t','e', ' ','9','0',0xb0,' ','C', 'W',  '\0' };
static unichar_t rotate90ccw[] = { 'R','o','t','a','t','e', ' ','9','0',0xb0,' ','C','C', 'W',  '\0' };
static unichar_t rotate180[] = { 'R','o','t','a','t','e', ' ','1', '8','0',0xb0,  '\0' };
static unichar_t skew[] = { 'S', 'k', 'e', 'w','.','.','.',  '\0' };
static unichar_t transform[] = { 'T','r','a','n','s','f','o','r', 'm',  '\0' };
unichar_t *BVFlipNames[] = { fliph, flipv, rotate90cw, rotate90ccw, rotate180, skew };

static GMenuItem dummyitem[] = { { new }, NULL };
static GMenuItem fllist[] = {
    { { new, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'N' }, 'N', ksm_control, NULL, NULL, MenuNew },
    { { open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, BVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { openoutline, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'u' }, 'H', ksm_control, NULL, NULL, BVMenuOpenOutline },
    { { openbitmap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 0, 'B' }, 'J', ksm_control, NULL, NULL, /* No function, never avail */NULL },
    { { openmetrics, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'M' }, 'K', ksm_control, NULL, NULL, BVMenuOpenMetrics },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'S' }, 'S', ksm_control, NULL, NULL, BVMenuSave },
    { { saveas, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, BVMenuSaveAs },
    { { generate, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'G' }, '\0', ksm_control|ksm_shift, NULL, NULL, BVMenuGenerate },
    { { export, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 't' }, '\0', ksm_control|ksm_shift, NULL, NULL, BVMenuExport },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { import, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'I' }, 'I', ksm_control|ksm_shift, NULL, NULL, BVMenuImport },
    { { revert, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, BVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'U' }, 'Z', ksm_control, NULL, NULL, BVUndo, MID_Undo },
    { { redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, 'Y', ksm_control, NULL, NULL, BVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 't' }, 'X', ksm_control, NULL, NULL, BVCut, MID_Cut },
    { { _copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, 'C', ksm_control, NULL, NULL, BVCopy, MID_Copy },
    { { paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 'V', ksm_control, NULL, NULL, BVPaste, MID_Paste },
    { { clear, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'l' }, GK_Delete, 0, NULL, NULL, BVClear, MID_Clear },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { selectall, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'A' }, 'A', ksm_control, NULL, NULL, BVSelectAll, MID_SelAll },
    { NULL }
};

static GMenuItem trlist[] = {
    { { fliph, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'H' }, '\0', ksm_control, NULL, NULL, BVMenuRotateInvoked, bvt_fliph },
    { { flipv, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'V' }, '\0', ksm_control, NULL, NULL, BVMenuRotateInvoked, bvt_flipv },
    { { rotate90cw, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, '\0', ksm_control, NULL, NULL, BVMenuRotateInvoked, bvt_rotate90cw },
    { { rotate90ccw, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, '9' }, '\0', ksm_control, NULL, NULL, BVMenuRotateInvoked, bvt_rotate90ccw },
    { { rotate180, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, '1' }, '\0', ksm_control, NULL, NULL, BVMenuRotateInvoked, bvt_rotate180 },
    { { skew, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'S' }, '\0', ksm_control, NULL, NULL, BVMenuRotateInvoked, bvt_skew },
    { NULL }
};

static GMenuItem ellist[] = {
    { { fontinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, 'F', ksm_control|ksm_shift, NULL, NULL, BVMenuFontInfo },
    { { privateinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 'P', ksm_control|ksm_shift, NULL, NULL, BVMenuPrivateInfo },
    { { getinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'I' }, 'I', ksm_control, NULL, NULL, BVMenuGetInfo, MID_GetInfo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { bitmapsavail, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'A' }, 'B', ksm_control|ksm_shift, NULL, NULL, BVMenuBitmaps, MID_AvailBitmaps },
    { { regenbitmaps, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'B' }, 'B', ksm_control, NULL, NULL, BVMenuBitmaps, MID_RegenBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { transform, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'T' }, '\0', 0, trlist, NULL },
    { NULL }
};

static GMenuItem pllist[] = {
    { { tools, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'T' }, '\0', ksm_control, NULL, NULL, BVMenuPaletteShow, MID_Tools },
    { { layers, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'L' }, '\0', ksm_control, NULL, NULL, BVMenuPaletteShow, MID_Layers },
    { NULL }
};

static GMenuItem vwlist[] = {
    { { fit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, 'F', ksm_control, NULL, NULL, BVMenuScale, MID_Fit },
    { { zoomout, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'i' }, '\0', ksm_control, NULL, NULL, BVMenuScale, MID_ZoomOut },
    { { zoomin, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'o' }, '\0', ksm_control, NULL, NULL, BVMenuScale, MID_ZoomIn },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { next, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'N' }, ']', ksm_control, NULL, NULL, BVMenuChangeChar, MID_Next },
    { { prev, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, '[', ksm_control, NULL, NULL, BVMenuChangeChar, MID_Prev },
    { { _goto, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'G' }, '>', ksm_shift|ksm_control, NULL, NULL, BVMenuGotoChar },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { bigger, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'B' }, '+', ksm_shift|ksm_control, NULL, NULL, BVMenuChangePixelSize, MID_Bigger },
    { { smaller, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'S' }, '-', ksm_control, NULL, NULL, BVMenuChangePixelSize, MID_Smaller },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { palettes, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'l' }, '\0', 0, pllist, pllistcheck },
    { NULL }
};

static GMenuItem mblist[] = {
    { { file, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'E' }, 0, 0, edlist, edlistcheck },
    { { element, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'l' }, 0, 0, ellist, NULL },
    { { view, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'W' }, 0, 0, NULL, WindowMenuBuild },
    { NULL }
};

#define bitmap_width 16
#define bitmap_height 16
static unsigned char bitmap_bits[] = {
   0x00, 0x00, 0xfc, 0x03, 0xfc, 0x03, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x0c,
   0x30, 0x0c, 0xf0, 0x03, 0xf0, 0x03, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x0c,
   0x30, 0x0c, 0xfc, 0x03, 0xfc, 0x03, 0x00, 0x00};

BitmapView *BitmapViewCreate(BDFChar *bc, BDFFont *bdf, FontView *fv) {
    BitmapView *bv = calloc(1,sizeof(BitmapView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    char buffer[210];
    GRect gsize;
    int sbsize;
    unichar_t ubuf[300];
    SplineChar *sc = bc->sc;
    static GWindow icon = NULL;
    GTextInfo ti;
    FontRequest rq;
    int as, ds, ld;
    static unichar_t fixed[] = { 'f','i','x','e','d', '\0' };

    BVShows.lastpixelsize = bdf->pixelsize;

    if ( icon==NULL )
	icon = GDrawCreateBitmap(NULL,bitmap_width,bitmap_height,bitmap_bits);

    bv->bc = bc;
    bv->scale = 1;
    bv->xoff = bv->yoff = 20;
    bv->next = bc->views;
    bc->views = bv;
    bv->fv = fv;
    bv->bdf = bdf;

    bv->showfore = BVShows.showfore;
    bv->showoutline = BVShows.showoutline;
    bv->showgrid = BVShows.showgrid;
    bv->scscale = ((double) (bdf->pixelsize))/(bdf->sf->ascent+bdf->sf->descent);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_ititle;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    sprintf( buffer, "%.90s at %d from %.90s ", bc->sc->name, bdf->pixelsize, fv->sf->fontname );
    wattrs.icon_title = uc_copy(buffer);
    uc_strcpy(ubuf,buffer);
    if ( sc->unicodeenc!=-1 && UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]!=NULL )
	u_strcat(ubuf, UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]);
    wattrs.window_title = ubuf;
    wattrs.icon = icon;
    if ( wattrs.icon )
	wattrs.mask |= wam_icon;
    pos.x = pos.y = 0; pos.width=270; pos.height = 250;

    bv->gw = gw = GDrawCreateTopWindow(NULL,&pos,bv_e_h,bv,&wattrs);
    free( wattrs.icon_title );

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    bv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(bv->mb,&gsize);
    bv->mbh = gsize.height;
    bv->infoh = GDrawPointsToPixels(gw,36);

    gd.pos.y = bv->mbh+bv->infoh;
    gd.pos.width = sbsize = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.height = pos.height-bv->mbh-bv->infoh - sbsize;
    gd.pos.x = pos.width-sbsize;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    bv->vsb = GScrollBarCreate(gw,&gd,bv);

    gd.pos.y = pos.height-sbsize; gd.pos.height = sbsize;
    gd.pos.width = pos.width - sbsize;
    gd.pos.x = 0;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    bv->hsb = GScrollBarCreate(gw,&gd,bv);

    memset(&gd, '\0', sizeof(gd));
    memset(&ti, '\0', sizeof(ti));
    gd.pos.x = pos.width - GDrawPointsToPixels(gw,116);
    gd.pos.y = bv->mbh + GDrawPointsToPixels(gw,6);
    gd.pos.width = GDrawPointsToPixels(gw,106);
    gd.label = &ti;
    ti.text = (unichar_t *) "Recalculate Bitmap";
    ti.text_is_1byte = true;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    if ( fv->sf->onlybitmaps )
	gd.flags = gg_pos_in_pixels;
    gd.handle_controlevent = BVRecalc;
    bv->recalc = GButtonCreate(gw,&gd,bv);

    pos.y = bv->mbh+bv->infoh; pos.height -= bv->mbh + sbsize + bv->infoh;
    pos.x = 0; pos.width -= sbsize;
    wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pencil;
    bv->v = GWidgetCreateSubWindow(gw,&pos,v_e_h,bv,&wattrs);

    bv->height = pos.height; bv->width = pos.width;
    bv->b1_tool = bvt_pencil; bv->cb1_tool = bvt_pointer;
    bv->b2_tool = bvt_magnify; bv->cb2_tool = bvt_shift;
    bv->showing_tool = bvt_pencil;
    bv->pressed_tool = bv->pressed_display = bv->active_tool = bvt_none;

    /*GWidgetHidePalettes();*/
    bv->tools = BVMakeTools(bv);
    bv->layers = BVMakeLayers(bv);

    memset(&rq,0,sizeof(rq));
    rq.family_name = fixed;
    rq.point_size = -7;
    rq.weight = 400;
    bv->small = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(bv->small,&as,&ds,&ld);
    bv->sfh = as+ds; bv->sas = as;

    BVFit(bv);
    GDrawSetVisible(bv->v,true);
    GDrawSetVisible(gw,true);
return( bv );
}

BitmapView *BitmapViewCreatePick(int enc, FontView *fv) {
    BDFFont *bdf;

    for ( bdf = fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize!=BVShows.lastpixelsize; bdf = bdf->next );
    if ( bdf==NULL && fv->show!=fv->filled )
	bdf = fv->show;
    if ( bdf==NULL )
	bdf = fv->sf->bitmaps;

    BDFMakeChar(bdf,enc);
return( BitmapViewCreate(bdf->chars[enc],bdf,fv));
}

void BitmapViewFree(BitmapView *bv) {
    free(bv);
}

