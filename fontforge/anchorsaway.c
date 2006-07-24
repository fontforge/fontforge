/* Copyright (C) 2005,2006 by George Williams */
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
#include <gkeysym.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

typedef struct anchord {
    GWindow gw;
    int ctl_len;
    GGadget *hsb;
    int sb_base;
    int sb_width;
    int sb_height;
    int full_width, full_height;

    SplineChar *sc;		/* This is the character whose anchor we are */
    AnchorPoint *ap;		/*  interested in, and this is that anchor */
    BasePoint apos;		/* Modified anchor position */
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    DeviceTable xadjust;	/* Modified adjustments */
    DeviceTable yadjust;
#endif
    BDFChar *bdfc;
    int char_off;		/* Mark characters are often positioned oddly */
    int char_size;		/*  and have zero advance widths, this corrects*/
    int ymin, ymax;		/* For all combos */

    int pixelsize;
    double scale;		/* Does not include mag factor */
    int magfactor;
    int baseline;

    int xoff;			/* for scroll bar */
    int xlen;			/* sum of all the following lengths	  */
    int cnt;			/* Number of anchors we join with	  */
	/* joining can happen in four ways:				  */
	/*   we might be a base char, we see all the accents		  */
	/*	(all apmatches will be the same length (bdfc->width above)*/
	/*   we might be a mark, we see all the bases (ligs may happen twice) */
	/*	(each base provides the length)				  */
	/*   we might be cursive entry, we see all the exits that attach  */
	/*	(length is bdfc->width + apmatch->width			  */
	/*	left to right/right to left is important here!		  */
	/*   we might be cursive exit, we see all the entries that attach */
    struct apmatch {
	SplineChar *sc;
	AnchorPoint *ap;
	BDFChar *bdfc;
	int off;		/* a mark-to-mark display might have the same */
	int size;		/* odd positioning problems as above */
	int xstart;		/* This is scaled by factor, others are not */
    } *apmatch;
    void *freetypecontext;
    int combo, on_ap;
    BasePoint orig_pos;
    int done;
} AnchorDlg;

#define CID_X		1001
#define CID_Y		1002
#define CID_XCor	1003
#define CID_YCor	1004
#define CID_DisplaySize	1005
#define CID_Mag		1006

static void AnchorD_FreeAll(AnchorDlg *a) {
    int i;

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    free(a->xadjust.corrections);
    free(a->yadjust.corrections);
#endif
    BDFCharFree(a->bdfc);
    for ( i=0; i<a->cnt; ++i )
	BDFCharFree(a->apmatch[i].bdfc);
    free(a->apmatch);
    if ( a->freetypecontext!=NULL )
	FreeTypeFreeContext(a->freetypecontext);
}

static void AnchorD_FindComplements(AnchorDlg *a) {
    AnchorClass *ac = a->ap->anchor;
    enum anchor_type match, match2;
    AnchorPoint *ap;
    int i, k, j, cnt;
    SplineFont *_sf = a->sc->parent, *sf;
    uint8 *sel, *oldsel;
    FontView *fv = _sf->fv;
    EncMap *map = fv->map;

    switch ( a->ap->type ) {
      case at_mark:
	match = at_basechar;
	match2 = at_baselig;
      break;
      case at_basechar: case at_baselig: case at_basemark:
	match = match2 = at_mark;
      break;
      case at_centry:
	match = match2 = at_cexit;
      break;
      case at_cexit:
	match = match2 = at_centry;
      break;
      default:
	match = match2 = at_max;
      break;
    }

    if ( _sf->cidmaster!=NULL )
	_sf = _sf->cidmaster;
    for ( j=0; j<2; ++j ) {
	k = cnt = 0;
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		for ( ap= sf->glyphs[i]->anchor; ap!=NULL; ap=ap->next ) {
		    if ( ap->anchor == ac && ( ap->type==match || ap->type==match2 )) {
			if ( j ) {
			    a->apmatch[cnt].ap = ap;
			    a->apmatch[cnt++].sc = sf->glyphs[i];
			} else
			    ++cnt;
			/* Don't break out of the loop as ligatures can have multiple locations */
		    }
		}
	    }
	    ++k;
	} while ( k<_sf->subfontcnt );
	a->cnt = cnt;
	if ( cnt==0 )
    break;
	if ( j==0 )
	    a->apmatch = gcalloc(cnt,sizeof(struct apmatch));
    }

    if ( hasFreeType() && _sf->subfontcnt==0 ) {
	int enc = map->backmap[a->sc->orig_pos];
	if ( enc!=-1 ) {
	    sel = gcalloc(map->enccount,1);
	    sel[enc] = true;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		enc = map->backmap[i];
		if ( enc!=-1 ) {
		    for ( ap= sf->glyphs[i]->anchor; ap!=NULL; ap=ap->next ) {
			if ( ap->anchor == ac && ( ap->type==match || ap->type==match2 )) {
			    sel[enc] = true;
		    break;
			}
		    }
		}
	    }
	    oldsel = fv->selected;
	    fv->selected = sel;
	    a->freetypecontext = FreeTypeFontContext(_sf,NULL,fv);
	    fv->selected = oldsel;
	    free(sel);
	}
    }
}

static BDFChar *APRasterize(void *freetypecontext, SplineChar *sc,int *off,int *size,int pixelsize) {
    BDFChar *bdfc;

    if ( freetypecontext ) {
	bdfc = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,pixelsize,8);
    } else
	bdfc = SplineCharAntiAlias(sc,pixelsize,4);

    if ( bdfc->xmin<=0 ) {
	*off = 1-bdfc->xmin;
	if ( bdfc->width>bdfc->xmax-bdfc->xmin )
	    *size = bdfc->width;
	else
	    *size = bdfc->xmax + *off;
    } else {
	*off = 0;
	if ( bdfc->width>bdfc->xmax )
	    *size = bdfc->width;
	else
	    *size = bdfc->xmax + 1;
    }
    ++*size;
return( bdfc );
}

static void AnchorD_SetSB(AnchorDlg *a) {
    GScrollBarSetBounds(a->hsb,0,a->xlen,a->sb_width);
    if ( a->xoff+a->sb_width > a->xlen )
	a->xoff = a->xlen - a->sb_width;
    if ( a->xoff<0 ) a->xoff = 0;
    GScrollBarSetPos(a->hsb,a->xoff);
}

static void AnchorD_Resize(AnchorDlg *a) {
    GRect size;

    GDrawGetSize(a->gw,&size);
    a->full_width = size.width;
    a->full_height = size.height;
    a->sb_base = a->full_height - a->sb_height;
    if ( a->ctl_len+a->magfactor*a->char_size + 20 < a->full_width ) {
	a->sb_width = a->full_width - (a->ctl_len+a->magfactor*a->char_size);
	GGadgetResize(a->hsb,a->sb_width, a->sb_height);
	GGadgetMove(a->hsb,a->ctl_len+a->magfactor*a->char_size,a->sb_base);
    }
    AnchorD_SetSB(a);
}

static void AnchorD_ChangeMag(AnchorDlg *a) {
    int i, extra;
    int factor = a->magfactor;

    for ( i=0; i<a->cnt; ++i ) {
	if ( i==0 )
	    a->apmatch[i].xstart = a->ctl_len+a->char_size*factor;
	else
	    a->apmatch[i].xstart = a->apmatch[i-1].xstart +
				    a->apmatch[i-1].size*factor;
    }
    if ( i==0 )
	a->xlen = 0;
    else
	a->xlen = a->apmatch[i-1].xstart - a->apmatch[0].xstart +
		a->apmatch[i-1].size*factor;

    if ( a->ymin>0 && a->sb_base- a->ymax*factor >=0 )
	extra = (a->sb_base - a->ymax*factor)/2;
    else
	extra = (a->sb_base - (a->ymax-a->ymin)*factor)/2;
    a->baseline = a->ymax*factor + extra;

    AnchorD_Resize(a);
}

static void AnchorD_ChangeSize(AnchorDlg *a) {
    int i, off;

    GDrawSetCursor(a->gw,ct_watch);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);

    a->scale = a->pixelsize / (double) (a->sc->parent->ascent + a->sc->parent->descent);

    BDFCharFree(a->bdfc);
    a->bdfc = APRasterize(a->freetypecontext,a->sc,&a->char_off,&a->char_size,a->pixelsize);
    a->ymin = a->bdfc->ymin; a->ymax = a->bdfc->ymax;
    for ( i=0; i<a->cnt; ++i ) {
	BDFCharFree(a->apmatch[i].bdfc);
	a->apmatch[i].bdfc = APRasterize(a->freetypecontext,a->apmatch[i].sc,&a->apmatch[i].off,&a->apmatch[i].size,a->pixelsize);
	if ( a->ap->type==at_centry || a->ap->type==at_cexit )
	    a->apmatch[i].size += a->char_size;
	else if ( a->ap->type!=at_mark )
	    a->apmatch[i].size = a->char_size;
	switch ( a->ap->type ) {
	  case at_cexit:
	  case at_basechar: case at_baselig: case at_basemark:
	    off = rint((-a->apos.y + a->apmatch[i].ap->me.y)*a->scale);
	    if ( a->apmatch[i].bdfc->ymax+off > a->ymax )
		a->ymax = a->apmatch[i].bdfc->ymax+off;
	    if ( a->apmatch[i].bdfc->ymin+off < a->ymin )
		a->ymin = a->apmatch[i].bdfc->ymin+off;
	  break;
	  case at_centry:
	  case at_mark:
	    if ( a->apmatch[i].bdfc->ymax > a->ymax )
		a->ymax = a->apmatch[i].bdfc->ymax;
	    if ( a->apmatch[i].bdfc->ymin < a->ymin )
		a->ymin = a->apmatch[i].bdfc->ymin;
	    off = rint((-a->apmatch[i].ap->me.y + a->apos.y)*a->scale);
	    if ( a->bdfc->ymax+off > a->ymax )
		a->ymax = a->bdfc->ymax+off;
	    if ( a->bdfc->ymin+off < a->ymin )
		a->ymin = a->bdfc->ymin+off;
	  break;
	}
    }
    AnchorD_ChangeMag(a);
    GDrawSetCursor(a->gw,ct_pointer);
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static int DevTabFind(DeviceTable *adjust,int pixelsize) {
    if ( adjust==NULL || adjust->corrections==NULL ||
	    pixelsize<adjust->first_pixel_size ||
	    pixelsize>adjust->last_pixel_size )
return( 0 );
return( adjust->corrections[pixelsize-adjust->first_pixel_size]);
}
#endif

static void AnchorD_Expose(AnchorDlg *a,GWindow pixmap,GEvent *event) {
    GRect *area = &event->u.expose.rect;
    GRect clip, old1, old2;
    int expose_end = area->x+area->width;
    int factor = a->magfactor;
    int i,x,y;

    if ( expose_end<a->ctl_len )
return;

    GDrawPushClip(pixmap,area,&old1);

    if ( area->x<a->ctl_len+a->char_size*factor ) {
	GDrawDrawLine(pixmap,a->ctl_len,0,a->ctl_len,a->full_height,0x000000);
	KCD_DrawGlyph(pixmap,a->char_off*factor+a->ctl_len,a->baseline,a->bdfc,factor);
	DrawAnchorPoint(pixmap,
	    a->char_off*factor+a->ctl_len + ((int) rint(a->apos.x*a->scale))*factor,
	    a->baseline - ((int) rint(a->apos.y*a->scale))*factor,
	    false);
	GDrawDrawLine(pixmap,a->ctl_len+a->char_size*factor,0,a->ctl_len+a->char_size*factor,a->full_height,0x000000);
    }

    if ( expose_end>a->ctl_len+a->char_size && area->y<a->sb_base ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	int xcor=0, ycor=0;
	unichar_t *end;
	const unichar_t *ret;

	ret = _GGadgetGetTitle(GWidgetGetControl(a->gw,CID_XCor));
	xcor = u_strtol(ret,&end,10);
	while ( *end==' ' ) ++end;
	if ( *end!='\0' || xcor<-128 || xcor>127 )
	    xcor = 0;
	ret = _GGadgetGetTitle(GWidgetGetControl(a->gw,CID_YCor));
	ycor = u_strtol(ret,&end,10);
	while ( *end==' ' ) ++end;
	if ( *end!='\0' || ycor<-128 || ycor>127 )
	    ycor = 0;
#endif
	
	clip = *area;
	if ( area->x<=a->ctl_len+a->char_size ) {
	    clip.width -= a->ctl_len+a->char_size+1 - area->x;
	    clip.x = a->ctl_len+a->char_size+1;
	}
	if ( area->y+area->height > a->sb_base )
	    clip.height = a->sb_base-area->y;
	GDrawPushClip(pixmap,&clip,&old2);
	for ( i=0; i<a->cnt; ++i ) {
	    if ( a->apmatch[i].xstart - a->xoff > clip.x+clip.width )
	break;
	    if ( a->apmatch[i].xstart+a->apmatch[i].size*factor - a->xoff < clip.x )
	continue;

	    y = a->baseline;
	    if ( i!=0 )
		GDrawDrawLine(pixmap,a->apmatch[i].xstart-a->xoff,0,
			a->apmatch[i].xstart-a->xoff,a->sb_base,0x808080);
	    if ( a->ap->type==at_cexit ||
		    a->ap->type==at_basechar || a->ap->type==at_baselig ||
		    a->ap->type==at_basemark ) {
		x = a->apmatch[i].xstart+a->char_off*factor - a->xoff;
		KCD_DrawGlyph(pixmap,x,y,a->bdfc,factor);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		x += ((int) rint((-a->apmatch[i].ap->me.x + a->apos.x)*a->scale) +
			xcor - DevTabFind(&a->apmatch[i].ap->xadjust,a->pixelsize))*factor;
		y += ((int) rint((a->apmatch[i].ap->me.y - a->apos.y)*a->scale) +
			-ycor + DevTabFind(&a->apmatch[i].ap->yadjust,a->pixelsize))*factor;
#else
		x += ((int) rint((-a->apmatch[i].ap->me.x + a->apos.x)*a->scale))*factor;
		y += ((int) rint((a->apmatch[i].ap->me.y - a->apos.y)*a->scale))*factor;
#endif
		KCD_DrawGlyph(pixmap,x,y,a->apmatch[i].bdfc,factor);
	    } else {
		x = a->apmatch[i].xstart+a->apmatch[i].off*factor - a->xoff;
		KCD_DrawGlyph(pixmap,x,y,a->apmatch[i].bdfc,factor);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		x += ((int) rint((a->apmatch[i].ap->me.x - a->apos.x)*a->scale) +
			DevTabFind(&a->apmatch[i].ap->xadjust,a->pixelsize)-xcor)*factor;
		y += ((int) rint((-a->apmatch[i].ap->me.y + a->apos.y)*a->scale) +
			-DevTabFind(&a->apmatch[i].ap->yadjust,a->pixelsize)+ycor)*factor;
#else
		x += ((int) rint((a->apmatch[i].ap->me.x - a->apos.x)*a->scale))*factor;
		y += ((int) rint((-a->apmatch[i].ap->me.y + a->apos.y)*a->scale))*factor;
#endif
		KCD_DrawGlyph(pixmap,x,y,a->bdfc,factor);
	    }
	}
	if ( i>0 )
	    GDrawDrawLine(pixmap,a->apmatch[i-1].xstart+a->apmatch[i-1].size*factor-a->xoff,0,
		    a->apmatch[i-1].xstart+a->apmatch[i-1].size*factor-a->xoff,a->sb_base,0x808080);
	GDrawPopClip(pixmap,&old2);
    }
    GDrawPopClip(pixmap,&old1);
}

static void AnchorD_FigurePos(AnchorDlg *a,GEvent *event) {
    int x = (event->u.mouse.x - a->ctl_len)/a->magfactor;
    int y = (a->baseline - event->u.mouse.y)/a->magfactor;

    a->apos.x = rint((x-a->char_off)/a->scale);
    a->apos.y = rint(y/a->scale);
}

static void AnchorD_ClearCorrections(AnchorDlg *a) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    unichar_t ubuf[2];

    ubuf[0] = '0'; ubuf[1] = '\0';
    free(a->xadjust.corrections); memset(&a->xadjust,0,sizeof(DeviceTable));
    free(a->yadjust.corrections); memset(&a->yadjust,0,sizeof(DeviceTable));
    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_XCor),ubuf);
    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_YCor),ubuf);
#endif
}

static void AnchorD_DrawPos(AnchorDlg *a) {
    GRect r;
    char buffer[20];
    unichar_t ubuf[20];

    sprintf( buffer, "%g", (double) a->apos.x );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_X),ubuf);
    sprintf( buffer, "%g", (double) a->apos.y );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_Y),ubuf);

    r.x = a->ctl_len;
    r.y = 0;
    r.width = a->char_size*a->magfactor;
    r.height = a->full_height;
    GDrawRequestExpose(a->gw,&r,false);
}

static int AnchorD_Mouse(AnchorDlg *a,GEvent *event) {
    int on_combo = -1;
    int on_ap = 0;
    int i;

    if ( event->u.mouse.x<a->ctl_len || event->u.mouse.y>=a->sb_base )
return( false );
    if ( a->xlen>0 && event->u.mouse.x>=a->apmatch[0].xstart ) {
	int x = event->u.mouse.x + a->xoff;
	for ( i=0; i<a->cnt; ++i ) {
	    if ( x>=a->apmatch[i].xstart &&
		    x<a->apmatch[i].xstart + a->apmatch[i].size*a->magfactor ) {
		on_combo = i;
	break;
	    }
	}
    } else if ( event->u.mouse.x>=a->ctl_len && event->u.mouse.x<a->ctl_len+ a->magfactor*a->char_size ) {
	int x = event->u.mouse.x - a->ctl_len;
	int y = a->baseline - event->u.mouse.y;
	int ax = (a->char_off + (int) rint(a->apos.x*a->scale))*a->magfactor;
	int ay = (int) rint(a->apos.y*a->scale)*a->magfactor;
	if ( x>ax-4 && x<ax+4 && y>ay-4 && y<ay+4 )
	    on_ap = 2;
	else
	    on_ap = 1;
    }

    if ( event->type == et_mousedown ) {
	if ( on_combo!=-1 )
	    a->combo = on_combo+1;
	else if ( on_ap==2 ) {
	    a->on_ap = true;
	    a->orig_pos = a->apos;
	}
    } else if ( event->type == et_mouseup ) {
	if ( on_combo!=-1 && on_combo+1==a->combo ) {
	    a->combo = 0;
	    GDrawSetCursor(a->gw,ct_watch);
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	    AnchorControl(a->apmatch[on_combo].sc,a->apmatch[on_combo].ap);
	    GDrawSetCursor(a->gw,ct_pointer);
	} else if ( on_ap && a->on_ap ) {
	    AnchorD_FigurePos(a,event);
	    AnchorD_ClearCorrections(a);
	    AnchorD_DrawPos(a);
	    GDrawRequestExpose(a->gw,NULL,false);
	} else if ( a->combo!=0 ) {
	} else if ( a->on_ap ) {
	    a->apos = a->orig_pos;
	    AnchorD_DrawPos(a);
	}
	a->on_ap = 0;
	a->combo = 0;
    } else if ( a->on_ap ) {
	AnchorD_FigurePos(a,event);
	AnchorD_DrawPos(a);
    }
return( true );
}

static void AnchorD_HScroll(AnchorDlg *a,struct sbevent *sb) {
    int newpos = a->xoff;
    GRect rect;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
	newpos -= a->sb_width;
      break;
      case et_sb_up:
        newpos -= a->pixelsize*a->magfactor/2;
      break;
      case et_sb_down:
        newpos = a->pixelsize*a->magfactor/2;
      break;
      case et_sb_downpage:
	newpos += a->sb_width;
      break;
      case et_sb_bottom:
        newpos = a->xlen - a->sb_width;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + a->sb_width >= a->xlen )
	newpos = a->xlen - a->sb_width;
    if ( newpos < 0 ) newpos = 0;
    if ( newpos!=a->xoff ) {
	int diff = newpos-a->xoff;
	a->xoff = newpos;
	GScrollBarSetPos(a->hsb,newpos);
	if ( a->cnt!=0 ) {
	    rect.x = a->apmatch[0].xstart+1; rect.y = 0;
	    rect.width = a->sb_width;
	    rect.height = a->sb_base;
	    GDrawScroll(a->gw,&rect,-diff,0);
	}
    }
}

static void AnchorD_DoCancel(AnchorDlg *a) {
    a->done = true;
}

static int anchord_e_h(GWindow gw, GEvent *event) {
    AnchorDlg *a = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	AnchorD_DoCancel(a);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("anchorcontrol.html");
return( true );
	}
return( false );
      break;
      case et_mouseup: case et_mousemove: case et_mousedown:
return( AnchorD_Mouse(a,event));
      break;
      case et_expose:
	AnchorD_Expose(a,gw,event);
      break;
      case et_resize:
	AnchorD_Resize(a);
	GDrawRequestExpose(a->gw,NULL,false);
      break;
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    AnchorD_HScroll(a,&event->u.control.u.sb);
	  break;
	}
      break;
    }
return( true );
}

static int AnchorD_MagnificationChanged(GGadget *g, GEvent *e) {
    AnchorDlg *a = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int mag = GGadgetGetFirstListSelectedItem(GWidgetGetControl(a->gw,CID_Mag));

	if ( mag!=-1 && mag!=a->magfactor-1 ) {
	    a->xoff = (mag+1)*a->xoff/a->magfactor;
	    a->magfactor = mag+1;
	    AnchorD_ChangeMag(a);
	    GDrawRequestExpose(a->gw,NULL,false);
	}
    }
return( true );
}

static int AnchorD_DisplaySizeChanged(GGadget *g, GEvent *e) {
    AnchorDlg *a = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(a->gw,CID_DisplaySize));
	unichar_t *end;
	int pixelsize = u_strtol(ret,&end,10);

	while ( *end==' ' ) ++end;
	if ( pixelsize>4 && pixelsize<400 && *end=='\0' ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    unichar_t ubuf[20]; char buffer[20];
	    ubuf[0] = '0'; ubuf[1] = '\0';
	    if ( a->xadjust.corrections!=NULL &&
		    pixelsize>=a->xadjust.first_pixel_size &&
		    pixelsize<=a->xadjust.last_pixel_size ) {
		sprintf( buffer, "%d", a->xadjust.corrections[
			pixelsize-a->xadjust.first_pixel_size]);
		uc_strcpy(ubuf,buffer);
	    }
	    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_XCor),ubuf);
	    ubuf[0] = '0'; ubuf[1] = '\0';
	    if ( a->yadjust.corrections!=NULL &&
		    pixelsize>=a->yadjust.first_pixel_size &&
		    pixelsize<=a->yadjust.last_pixel_size ) {
		sprintf( buffer, "%d", a->yadjust.corrections[
			pixelsize-a->yadjust.first_pixel_size]);
		uc_strcpy(ubuf,buffer);
	    }
	    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_YCor),ubuf);
#endif
	    a->xoff = pixelsize*a->xoff/a->pixelsize;
	    a->pixelsize = pixelsize;
	    AnchorD_ChangeSize(a);
	    GDrawRequestExpose(a->gw,NULL,false);
	}
    }
return( true );
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static int AnchorD_CorrectionChanged(GGadget *g, GEvent *e) {
    AnchorDlg *a = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *ret = _GGadgetGetTitle(g);
	int is_y = GGadgetGetCid(g)==CID_YCor;
	unichar_t *end;
	int correction = u_strtol(ret,&end,10);

	while ( *end==' ' ) ++end;
	if ( *end!='\0' )
return( true );
	if ( correction<-128 || correction>127 ) {
#if !defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)
	    gwwv_post_error( _("Out of Range"), _("Corrections must be between -128 and 127 (and should be smaller)") );
#endif
return( true );
	}

	DeviceTableSet(is_y?&a->yadjust:&a->xadjust,a->pixelsize,correction);
	GDrawRequestExpose(a->gw,NULL,false);
    }
return( true );
}
#endif

static int AnchorD_PositionChanged(GGadget *g, GEvent *e) {
    AnchorDlg *a = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *ret = _GGadgetGetTitle(g);
	int is_y = GGadgetGetCid(g)==CID_Y;
	unichar_t *end;
	int val = u_strtol(ret,&end,10);

	while ( *end==' ' ) ++end;
	if ( *end!='\0' )
return( true );

	if ( (is_y && a->apos.y==val) || (!is_y && a->apos.x==val))
return( true );
	if ( is_y )
	    a->apos.y = val;
	else
	    a->apos.x = val;
	AnchorD_ClearCorrections(a);
	GDrawRequestExpose(a->gw,NULL,false);
    }
return( true );
}

static int AnchorD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	AnchorDlg *a = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorD_DoCancel(a);
    }
return( true );
}

static int AnchorD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	AnchorDlg *a = GDrawGetUserData(GGadgetGetWindow(g));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( a->xadjust.corrections==NULL ) {
	    free(a->ap->xadjust.corrections);
	    memset(&a->ap->xadjust,0,sizeof(DeviceTable));
	} else {
	    a->ap->xadjust = a->xadjust;
	    a->xadjust.corrections = NULL;
	}
	if ( a->yadjust.corrections==NULL ) {
	    free(a->ap->yadjust.corrections);
	    memset(&a->ap->yadjust,0,sizeof(DeviceTable));
	} else {
	    a->ap->yadjust = a->yadjust;
	    a->yadjust.corrections = NULL;
	}
#endif
	a->ap->me = a->apos;
	a->done = true;
	SCCharChangedUpdate(a->sc);
    }
return( true );
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static void AnchorD_SetDevTabs(AnchorDlg *a) {
    char buffer[20];
    unichar_t ubuf[20];
    int min, max;

    GGadgetClearList(GWidgetGetControl(a->gw,CID_DisplaySize));
    min = 1; max = 0;
    if ( a->xadjust.corrections!=NULL ) {
	min = a->xadjust.first_pixel_size;
	max = a->xadjust.last_pixel_size;
    }
    if ( a->yadjust.corrections!=NULL ) {
	if ( a->yadjust.first_pixel_size<min ) min = a->yadjust.first_pixel_size;
	if ( a->yadjust.last_pixel_size>max ) max = a->yadjust.last_pixel_size;
    }
    if ( max>min ) {
	int i;
	int len = max-min+1;
	char buffer[20];
	GTextInfo **ti = galloc((len+1)*sizeof(GTextInfo *));
	for ( i=0; i<len; ++i ) {
	    ti[i] = gcalloc(1,sizeof(GTextInfo));
	    sprintf( buffer, "%d", i+min );
	    ti[i]->text = uc_copy(buffer);
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	}
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	GGadgetSetList(GWidgetGetControl(a->gw,CID_DisplaySize),ti,false);
    }
    ubuf[0] = '0'; ubuf[1] = '\0';
    if ( a->xadjust.corrections!=NULL &&
	    a->pixelsize>=a->xadjust.first_pixel_size &&
	    a->pixelsize<=a->xadjust.last_pixel_size ) {
	sprintf( buffer, "%d", a->xadjust.corrections[
		a->pixelsize-a->xadjust.first_pixel_size]);
	uc_strcpy(ubuf,buffer);
    }
    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_XCor),ubuf);
    ubuf[0] = '0'; ubuf[1] = '\0';
    if ( a->yadjust.corrections!=NULL &&
	    a->pixelsize>=a->yadjust.first_pixel_size &&
	    a->pixelsize<=a->yadjust.last_pixel_size ) {
	sprintf( buffer, "%d", a->yadjust.corrections[
		a->pixelsize-a->yadjust.first_pixel_size]);
	uc_strcpy(ubuf,buffer);
    }
    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_YCor),ubuf);
}
#endif

static GTextInfo magnifications[] = {
    { (unichar_t *) "100%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) "200%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "300%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "400%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

void AnchorControl(SplineChar *sc,AnchorPoint *ap) {
    GRect pos;
    GWindowAttrs wattrs;
    AnchorDlg a;
    GWindow gw;
    GGadgetCreateData gcd[30];
    GTextInfo label[30];
    int k;
    extern int _GScrollBar_Width;
    char buffer[20], xbuf[20], ybuf[20];

    memset(&a,0,sizeof(a));
    a.sc = sc;
    a.ap = ap;
    a.apos = ap->me;
    a.pixelsize = 150;
    a.magfactor = 1;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( ap->xadjust.corrections!=NULL ) {
	int len = ap->xadjust.last_pixel_size-ap->xadjust.first_pixel_size+1;
	a.xadjust = ap->xadjust;
	a.xadjust.corrections = galloc(len);
	memcpy(a.xadjust.corrections,ap->xadjust.corrections,len);
    }
    if ( ap->yadjust.corrections!=NULL ) {
	int len = ap->yadjust.last_pixel_size-ap->yadjust.first_pixel_size+1;
	a.yadjust = ap->yadjust;
	a.yadjust.corrections = galloc(len);
	memcpy(a.yadjust.corrections,ap->yadjust.corrections,len);
    }
#endif

    memset(&wattrs,0,sizeof(wattrs));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Anchor Control...");
    wattrs.is_dlg = true;
    GDrawGetSize(GDrawGetRoot(NULL),&pos);
    pos.x = pos.y = 0;
    pos.height = GDrawPointsToPixels(NULL,210);
    pos.width -= 50;
    a.gw = gw = GDrawCreateTopWindow(NULL,&pos,anchord_e_h,&a,&wattrs);

    a.ctl_len = GDrawPointsToPixels(gw,140);
    a.sb_height = GDrawPointsToPixels(gw,_GScrollBar_Width);
    a.sb_base = pos.height - a.sb_height;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    label[k].text = (unichar_t *) _("_Size:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 9;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    sprintf( buffer, "%d", a.pixelsize );
    label[k].text = (unichar_t *) buffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 40; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_DisplaySize;
    gcd[k].gd.handle_controlevent = AnchorD_DisplaySizeChanged;
#ifndef FONTFORGE_CONFIG_DEVICETABLES
    gcd[k++].creator = GTextFieldCreate;
#else
    gcd[k++].creator = GListFieldCreate;
#endif

/* GT: Short for: Magnification */
    label[k].text = (unichar_t *) _("Mag:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+26;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 45; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Mag;
    gcd[k].gd.u.list = magnifications;
    gcd[k].gd.handle_controlevent = AnchorD_MagnificationChanged;
    gcd[k++].creator = GListButtonCreate;

    label[k].text = (unichar_t *) _("_X");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    sprintf( xbuf, "%d", (int) rint(ap->me.x) );
    label[k].text = (unichar_t *) xbuf;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 40; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_X;
    gcd[k].gd.handle_controlevent = AnchorD_PositionChanged;
    gcd[k++].creator = GTextFieldCreate;

#ifdef FONTFORGE_CONFIG_DEVICETABLES
/* GT: Short for Correction */
    label[k].text = (unichar_t *) _("Cor:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_utf8_popup ;
    gcd[k].gd.popup_msg = (unichar_t *) _("Correction in pixels to the horizontal positioning of this anchor point\nwhen rasterizing at the given pixelsize.\n(Lives in a Device Table)");
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) "0";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 45; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_XCor;
    gcd[k].gd.handle_controlevent = AnchorD_CorrectionChanged;
    gcd[k++].creator = GTextFieldCreate;
#endif

    label[k].text = (unichar_t *) _("_Y");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    sprintf( ybuf, "%d", (int) rint(ap->me.y) );
    label[k].text = (unichar_t *) ybuf;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 40; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Y;
    gcd[k].gd.handle_controlevent = AnchorD_PositionChanged;
    gcd[k++].creator = GTextFieldCreate;

#ifdef FONTFORGE_CONFIG_DEVICETABLES
/* GT: Short for Correction */
    label[k].text = (unichar_t *) _("Cor:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_utf8_popup ;
    gcd[k].gd.popup_msg = (unichar_t *) _("Correction in pixels to the horizontal positioning of this anchor point\nwhen rasterizing at the given pixelsize.\n(Lives in a Device Table)");
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) "0";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 45; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_YCor;
    gcd[k].gd.handle_controlevent = AnchorD_CorrectionChanged;
    gcd[k++].creator = GTextFieldCreate;
#endif

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+40;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_but_default;
    gcd[k].gd.handle_controlevent = AnchorD_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_but_cancel;
    gcd[k].gd.handle_controlevent = AnchorD_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 300;
    gcd[k].gd.pos.y = pos.height-a.sb_height;
    gcd[k].gd.pos.height = a.sb_height;
    gcd[k].gd.pos.width = pos.width-300;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gcd[k++].creator = GScrollBarCreate;

    GGadgetsCreate(a.gw,gcd);
    a.hsb = gcd[k-1].ret;

    AnchorD_FindComplements(&a);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    AnchorD_SetDevTabs(&a);
#endif
    AnchorD_ChangeSize(&a);

    GDrawSetVisible(a.gw,true);
    while ( !a.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(a.gw);
    AnchorD_FreeAll(&a);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
