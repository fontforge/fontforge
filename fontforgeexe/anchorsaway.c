/* Copyright (C) 2005-2012 by George Williams */
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
#include "fvfonts.h"
#include <gkeysym.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

int aa_pixelsize = 150;

/* The dialog should contain a lig index !!!! */

struct apmatch {
    SplineChar *sc;
    AnchorPoint *ap;
    BDFChar *bdfc;
    int off;		/* a mark-to-mark display might have the same */
    int size;		/* odd positioning problems as above */
    int xstart;		/* This is scaled by factor, others are not */
};

struct state {
    SplineChar *sc;
    int changed;
    AnchorPoint *ap_pt;
    AnchorPoint ap_vals;
    struct state *next;
};

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
    DeviceTable xadjust;	/* Modified adjustments */
    DeviceTable yadjust;
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
    struct apmatch *apmatch;
    void *freetypecontext;
    int combo, on_ap;
    BasePoint orig_pos;
    int done;
    /* If they change more than one anchor, retain the original values so that */
    /*  we can revert to them */
    struct state *orig_vals;
    int layer;
} AnchorDlg;

#define CID_X		1001
#define CID_Y		1002
#define CID_XCor	1003
#define CID_YCor	1004
#define CID_DisplaySize	1005
#define CID_Mag		1006
#define CID_Glyph	1007

#define Add_Mark	(void *) (-1)
#define Add_Base	(void *) (-3)

static void AnchorD_FreeChar(AnchorDlg *a) {
    int i;

    BDFCharFree(a->bdfc); a->bdfc = NULL;
    for ( i=0; i<a->cnt; ++i )
	BDFCharFree(a->apmatch[i].bdfc);
    free(a->apmatch); a->apmatch = NULL;
    if ( a->freetypecontext!=NULL ) {
        FreeTypeFreeContext(a->freetypecontext);
        a->freetypecontext = NULL;
    }
}

static void AnchorD_FreeAll(AnchorDlg *a) {
    struct state *old, *oldnext;

    free(a->xadjust.corrections);
    free(a->yadjust.corrections);
    for ( old = a->orig_vals; old!=NULL; old=oldnext ) {
	oldnext = old->next;
	chunkfree(old,sizeof(struct state));
    }
    AnchorD_FreeChar(a);
}

static GTextInfo **AnchorD_GlyphsInClass(AnchorDlg *a) {
    SplineFont *_sf = a->sc->parent, *sf;
    AnchorClass *ac = a->ap->anchor;
    int bcnt, mcnt, btot=0, j, k, gid;
    GTextInfo **ti;
    AnchorPoint *ap;

    if ( _sf->cidmaster!=NULL )
	_sf = _sf->cidmaster;
    for ( j=0; j<2; ++j ) {
	k = 0;
	bcnt = mcnt = 1;	/* Reserve two spots for labels (marks vs base glyphs) */
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
		for ( ap= sf->glyphs[gid]->anchor; ap!=NULL; ap=ap->next ) {
		    if ( ap->anchor == ac ) {
			if ( ap->type==at_mark || ap->type == at_centry ) {
			    if ( j ) {
				ti[btot+1+mcnt] = calloc(1,sizeof(GTextInfo));
				ti[btot+1+mcnt]->text = utf82u_copy(sf->glyphs[gid]->name);
				ti[btot+1+mcnt]->fg = ti[btot+1+mcnt]->bg = COLOR_DEFAULT;
			        ti[btot+1+mcnt]->userdata = ap;
			        ti[btot+1+mcnt]->selected = a->ap == ap;
			    }
			    ++mcnt;
			} else {
			    if ( j ) {
				ti[bcnt] = calloc(1,sizeof(GTextInfo));
				ti[bcnt]->text = utf82u_copy(sf->glyphs[gid]->name);
				ti[bcnt]->fg = ti[bcnt]->bg = COLOR_DEFAULT;
			        ti[bcnt]->userdata = ap;
			        ti[bcnt]->selected = a->ap == ap;
			    }
			    ++bcnt;
			}
		    }
		}
	    }
	    ++k;
	} while ( k<_sf->subfontcnt );
	if ( !j ) {
	    btot = bcnt;
	    ti = calloc(bcnt+mcnt+5,sizeof(GTextInfo*));
	    ti[0] = calloc(1,sizeof(GTextInfo));
	    ti[0]->text = utf82u_copy(ac->type==act_curs ? _("Exits") : _("Bases"));
	    ti[0]->fg = ti[0]->bg = COLOR_DEFAULT;
	    ti[0]->disabled = true;
	    ti[btot] = calloc(1,sizeof(GTextInfo));
	    ti[btot]->line = true;
	    ti[btot]->fg = ti[btot]->bg = COLOR_DEFAULT;
	    ti[btot+1] = calloc(1,sizeof(GTextInfo));
	    ti[btot+1]->text = utf82u_copy(ac->type==act_curs ? _("Entries") : _("Marks"));
	    ti[btot+1]->fg = ti[btot+1]->bg = COLOR_DEFAULT;
	    ti[btot+1]->disabled = true;
	    ti[btot+mcnt+1] = calloc(1,sizeof(GTextInfo));
	    ti[btot+mcnt+1]->line = true;
	    ti[btot+mcnt+1]->fg = ti[btot+mcnt+1]->bg = COLOR_DEFAULT;
	    ti[btot+mcnt+2] = calloc(1,sizeof(GTextInfo));
	    ti[btot+mcnt+2]->text = utf82u_copy(ac->type==act_curs ? _("Add Exit Anchor...") : _("Add Base Anchor..."));
	    ti[btot+mcnt+2]->fg = ti[btot+mcnt+2]->bg = COLOR_DEFAULT;
	    ti[btot+mcnt+2]->userdata = Add_Base;
	    ti[btot+mcnt+3] = calloc(1,sizeof(GTextInfo));
	    ti[btot+mcnt+3]->text = utf82u_copy(ac->type==act_curs ? _("Add Entry Anchor...") : _("Add Mark Anchor..."));
	    ti[btot+mcnt+3]->fg = ti[btot+mcnt+3]->bg = COLOR_DEFAULT;
	    ti[btot+mcnt+3]->userdata = Add_Mark;
	    ti[btot+mcnt+4] = calloc(1,sizeof(GTextInfo));
	}
    }
return( ti );
}

static void AnchorD_SetTitle(AnchorDlg *a) {
    char buffer[300];
    snprintf(buffer,sizeof(buffer),_("Anchor Control for class %.100s in glyph %.100s as %.20s"),
	    a->ap->anchor->name, a->sc->name,
	    a->ap->type == at_mark ? _("mark") :
	    a->ap->type == at_centry ? _("cursive entry") :
	    a->ap->type == at_cexit ? _("cursive exit") :
		_("base") );
    GDrawSetWindowTitles8(a->gw,buffer,_("Anchor Control"));
}
    
static void AnchorD_FindComplements(AnchorDlg *a) {
    AnchorClass *ac = a->ap->anchor;
    enum anchor_type match;
    AnchorPoint *ap;
    int i, k, j, cnt;
    SplineFont *_sf = a->sc->parent, *sf;
    uint8 *sel, *oldsel;
    FontView *fv = (FontView *) _sf->fv;
    EncMap *map = fv->b.map;

    switch ( a->ap->type ) {
      case at_mark:
        switch ( a->ap->anchor->type ) {
	  case act_mark:
	    match = at_basechar;
	  break;
	  case act_mklg:
	    match = at_baselig;
	  break;
	  case act_mkmk:
	    match = at_basemark;
	  break;
	  default:
	    IError( "Unexpected anchor class type" );
	    match = at_basechar;
	}
      break;
      case at_basechar: case at_baselig: case at_basemark:
	match = at_mark;
      break;
      case at_centry:
	match = at_cexit;
      break;
      case at_cexit:
	match = at_centry;
      break;
      default:
	match = at_max;
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
		    if ( ap->anchor == ac && ap->type==match ) {
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
	    a->apmatch = calloc(cnt,sizeof(struct apmatch));
    }

    if ( hasFreeType() && _sf->subfontcnt==0 ) {
	int enc = map->backmap[a->sc->orig_pos];
	if ( enc!=-1 ) {
	    sel = calloc(map->enccount,1);
	    sel[enc] = true;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		enc = map->backmap[i];
		if ( enc!=-1 ) {
		    for ( ap= sf->glyphs[i]->anchor; ap!=NULL; ap=ap->next ) {
			if ( ap->anchor == ac && ap->type==match ) {
			    sel[enc] = true;
		    break;
			}
		    }
		}
	    }
	    oldsel = fv->b.selected;
	    fv->b.selected = sel;
	    a->freetypecontext = FreeTypeFontContext(_sf,NULL,(FontViewBase *) fv,a->layer);
	    fv->b.selected = oldsel;
	    free(sel);
	}
    }
}

static BDFChar *APRasterize(void *freetypecontext, SplineChar *sc,int layer, int *off,int *size,int pixelsize) {
    BDFChar *bdfc;

    if ( freetypecontext ) {
	bdfc = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,pixelsize,72,8);
    } else
	bdfc = SplineCharAntiAlias(sc,layer,pixelsize,4);

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
    int i, extra;
    int factor = a->magfactor;

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

    GDrawRequestExpose(a->gw,NULL,false);
}

static void AnchorD_ChangeMag(AnchorDlg *a) {

    AnchorD_Resize(a);
}

static void AnchorD_ChangeSize(AnchorDlg *a) {
    int i, off;

    GDrawSetCursor(a->gw,ct_watch);
    GDrawSync(NULL);
    /* GDrawProcessPendingEvents(NULL); */ /* Any expose events on the current dlg will cause a crash as there are no bdfc's now */

    a->scale = a->pixelsize / (double) (a->sc->parent->ascent + a->sc->parent->descent);

    BDFCharFree(a->bdfc);
    a->bdfc = APRasterize(a->freetypecontext,a->sc,a->layer,&a->char_off,&a->char_size,a->pixelsize);
    a->ymin = a->bdfc->ymin; a->ymax = a->bdfc->ymax;
    for ( i=0; i<a->cnt; ++i ) {
	BDFCharFree(a->apmatch[i].bdfc);
	a->apmatch[i].bdfc = APRasterize(a->freetypecontext,a->apmatch[i].sc,a->layer,&a->apmatch[i].off,&a->apmatch[i].size,a->pixelsize);
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

static int DevTabFind(DeviceTable *adjust,int pixelsize) {
    if ( adjust==NULL || adjust->corrections==NULL ||
	    pixelsize<adjust->first_pixel_size ||
	    pixelsize>adjust->last_pixel_size )
return( 0 );
return( adjust->corrections[pixelsize-adjust->first_pixel_size]);
}

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
		x += ((int) rint((-a->apmatch[i].ap->me.x + a->apos.x)*a->scale) +
			xcor - DevTabFind(&a->apmatch[i].ap->xadjust,a->pixelsize))*factor;
		y += ((int) rint((a->apmatch[i].ap->me.y - a->apos.y)*a->scale) +
			-ycor + DevTabFind(&a->apmatch[i].ap->yadjust,a->pixelsize))*factor;
		KCD_DrawGlyph(pixmap,x,y,a->apmatch[i].bdfc,factor);
	    } else {
		x = a->apmatch[i].xstart+a->apmatch[i].off*factor - a->xoff;
		KCD_DrawGlyph(pixmap,x,y,a->apmatch[i].bdfc,factor);
		x += ((int) rint((a->apmatch[i].ap->me.x - a->apos.x)*a->scale) +
			DevTabFind(&a->apmatch[i].ap->xadjust,a->pixelsize)-xcor)*factor;
		y += ((int) rint((-a->apmatch[i].ap->me.y + a->apos.y)*a->scale) +
			-DevTabFind(&a->apmatch[i].ap->yadjust,a->pixelsize)+ycor)*factor;
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
    unichar_t ubuf[2];

    ubuf[0] = '0'; ubuf[1] = '\0';
    free(a->xadjust.corrections); memset(&a->xadjust,0,sizeof(DeviceTable));
    free(a->yadjust.corrections); memset(&a->yadjust,0,sizeof(DeviceTable));
    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_XCor),ubuf);
    GGadgetSetTitle(GWidgetGetControl(a->gw,CID_YCor),ubuf);
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

static void AnchorD_SelectGlyph(AnchorDlg *a, AnchorPoint *ap) {
    int i;
    int32 len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(a->gw,CID_Glyph),&len);

    for ( i=0; i<len; ++i )
	if ( ti[i]->userdata == ap )
    break;
    if ( i!=len )
	GGadgetSelectOneListItem(GWidgetGetControl(a->gw,CID_Glyph),i);
}

static int AnchorD_ChangeGlyph(AnchorDlg *a, SplineChar *sc, AnchorPoint *ap);

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
	    AnchorPoint *ap = a->apmatch[on_combo].ap;
	    a->combo = 0;
	    AnchorD_ChangeGlyph(a,a->apmatch[on_combo].sc,a->apmatch[on_combo].ap);
	    AnchorD_SelectGlyph(a,ap);
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

static SplinePoint *SCFindPoint(SplineChar *sc,int layer,int ptnum) {
    /* We're going to want to move this point, so a point in a reference */
    /*  is totally useless to us (we'd have to move the entire reference */
    /*  and it's probably better just to detach the anchor in that case. */
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->ttfindex==ptnum )
return( sp );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
return( NULL );
}

static void SetAnchor(SplineChar *sc,int layer, AnchorPoint *ap,DeviceTable *xadjust, DeviceTable *yadjust, BasePoint *pos) {
    int ly;

    free(ap->xadjust.corrections);
    if ( xadjust->corrections==NULL ) {
	memset(&ap->xadjust,0,sizeof(DeviceTable));
    } else {
	ap->xadjust = *xadjust;
	xadjust->corrections = NULL;
    }
    free(ap->yadjust.corrections);
    if ( yadjust->corrections==NULL ) {
	memset(&ap->yadjust,0,sizeof(DeviceTable));
    } else {
	ap->yadjust = *yadjust;
	yadjust->corrections = NULL;
    }
    ap->me = *pos;
    /* If the anchor is bound to a truetype point we must move the point too */
    /*  or the anchor will just snap back to the point */
    ly = ly_none;
    if ( ap->has_ttf_pt && ap->ttf_pt_index!=0xffff ) {
	int any = false;
	for ( ly=ly_fore; ly<sc->layer_cnt; ++ly ) if ( sc->layers[ly].order2 ) {
	    SplinePoint *sp = SCFindPoint(sc,layer,ap->ttf_pt_index);
	    if ( sp!=NULL ) {
		sp->nextcp.x += pos->x - sp->me.x;
		sp->prevcp.x += pos->x - sp->me.x;
		sp->nextcp.y += pos->y - sp->me.y;
		sp->prevcp.y += pos->y - sp->me.y;
		sp->me = *pos;
		any = true;
	    }
	}
	if ( !any ) {
	    ff_post_notice(_("Detaching Anchor Point"),_("This anchor was attached to point %d, but that's not a point I can move. I'm detaching the anchor from the point.") );
	    ap->has_ttf_pt = false;
	    ly = ly_none;
	}
    }
    SCCharChangedUpdate(sc,ly);
}

static void AnchorD_DoCancel(AnchorDlg *a) {
    struct state *old;

    for ( old = a->orig_vals; old!=NULL; old=old->next ) {
	SetAnchor(old->sc,a->layer,old->ap_pt,&old->ap_vals.xadjust,&old->ap_vals.yadjust,&old->ap_vals.me);
	old->ap_pt->has_ttf_pt = old->ap_vals.has_ttf_pt;
	old->sc->changed = old->changed;	/* Must come after the charchangedupdate */
    }
    if ( a->orig_vals!=NULL ) {
	FVRefreshAll(a->sc->parent);
    }
	
    a->done = true;
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
	    a->xoff = pixelsize*a->xoff/a->pixelsize;
	    a->pixelsize = aa_pixelsize = pixelsize;
	    AnchorD_ChangeSize(a);
	    GDrawRequestExpose(a->gw,NULL,false);
	}
    }
return( true );
}

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
	    ff_post_error( _("Out of Range"), _("Corrections must be between -128 and 127 (and should be smaller)") );
return( true );
	}

	DeviceTableSet(is_y?&a->yadjust:&a->xadjust,a->pixelsize,correction);
	GDrawRequestExpose(a->gw,NULL,false);
    }
return( true );
}

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

static int AnchorD_Apply(AnchorDlg *a) {
    SetAnchor(a->sc,a->layer,a->ap,&a->xadjust,&a->yadjust,&a->apos);
return( true );
}

static int AnchorD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	AnchorDlg *a = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorD_Apply(a);
	a->done = true;
    }
return( true );
}

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
	GTextInfo **ti = malloc((len+1)*sizeof(GTextInfo *));
	for ( i=0; i<len; ++i ) {
	    ti[i] = calloc(1,sizeof(GTextInfo));
	    sprintf( buffer, "%d", i+min );
	    ti[i]->text = uc_copy(buffer);
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	}
	ti[i] = calloc(1,sizeof(GTextInfo));
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

static int AnchorD_ChangeGlyph(AnchorDlg *a, SplineChar *sc, AnchorPoint *ap) {
    char buf[32];
    struct state *old;

    if ( ap==NULL || sc==NULL )
return( true );
    if ( a->ap==ap )
return( true );
    /* Do we already have an entry for the current anchor? */
    for ( old = a->orig_vals; old!=NULL && old->ap_pt!=a->ap; old=old->next );
    /* If so we've already noted its original state and need do nothing more */
    /*  but otherwise we must store the current state */
    if ( old==NULL ) {
	old = chunkalloc(sizeof(struct state));
	old->sc = a->sc;
	old->changed = a->sc->changed;
	old->ap_pt = a->ap;
	old->ap_vals = *a->ap;
	memset(&a->ap->xadjust,0,sizeof(DeviceTable));
	memset(&a->ap->yadjust,0,sizeof(DeviceTable));
	old->next = a->orig_vals;
	a->orig_vals = old;
    }
    AnchorD_Apply(a);
    AnchorD_FreeChar(a);

    a->ap = ap;
    a->sc = sc;
    a->apos = ap->me;
    sprintf( buf, "%d", (int) rint(ap->me.x) );
    GGadgetSetTitle8(GWidgetGetControl(a->gw,CID_X),buf);
    sprintf( buf, "%d", (int) rint(ap->me.y) );
    GGadgetSetTitle8(GWidgetGetControl(a->gw,CID_Y),buf);

    AnchorD_FindComplements(a);
    AnchorD_SetDevTabs(a);
    AnchorD_ChangeSize(a);
    AnchorD_SetTitle(a);
return( true );
}

static SplineChar *AddAnchor(AnchorDlg *a, SplineFont *sf, AnchorClass *ac,
	int ismarklike) {
    char *ret, *def;
    SplineChar *sc;
    int isliga = false, ismrk=false, maxlig=-1;
    AnchorPoint *ap;
    PST *pst;
    int i;

    def = copy(".notdef");
    for (;;) {
	ret = gwwv_ask_string(_("Provide a glyph name"),def,_("Please identify a glyph by name, and FontForge will add an anchor to that glyph."));
	free(def);
	if ( ret==NULL )
return( NULL );
	sc = SFGetChar(sf,-1,ret);
	def = ret;
	if ( sc==NULL )
	    ff_post_error(_("Non-existant glyph"), _("The glyph, %.80s, is not in the font"), ret );
	else {
	    isliga = ismrk = false;
	    for ( ap=sc->anchor ; ap!=NULL; ap=ap->next ) {
		if ( ap->type == at_baselig )
		    isliga = true;
		else if ( ap->type == at_basemark || ap->type == at_mark )
		    ismrk = true;
		if ( ap->anchor == ac ) {
		    if ( (ap->type == at_centry ||
			    (ap->type == at_mark && ac->type==act_mkmk) ||
			    ap->type == at_baselig ) && ismarklike==-1 )
			ismarklike = false;
		    else if ( (ap->type == at_cexit || (ap->type == at_basemark && ac->type==act_mkmk)) && ismarklike==-1 )
			ismarklike = true;
		    else if ( ap->type != at_baselig ||
			      ( ap->type == at_baselig && ismarklike>0 ))
			ff_post_error(_("Duplicate Anchor Class"), _("The glyph, %.80s, already contains an anchor in this class, %.80s."), ret, ac->name );
		    else if ( maxlig<ap->lig_index )
			maxlig = ap->lig_index;
	    break;
		}
	    }
	    if ( ap==NULL )
    break;
	}
    }

    ap = chunkalloc(sizeof(AnchorPoint));
    ap->anchor = ac;
    ap->me.x = ap->me.y = 0;
    ap->next = sc->anchor;
    sc->anchor = ap;
    SCCharChangedUpdate(sc,ly_none);

    if ( sc->width==0 ) ismrk = true;
    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_ligature || pst->type == pst_lcaret ) {
	    isliga = true;
    break;
	}
    }

    if ( isliga || (ac->type==act_mklg && ismarklike==0 ) ) {
	ap->type = at_baselig;
	ap->lig_index = maxlig+1;
    } else if ( ismrk && ismarklike!=0 )
	ap->type = at_mark;
    else if ( ismarklike==0 && (ismrk || ac->type==act_mkmk) )
	ap->type = at_basemark;
    else if ( ac->type == act_curs ) {
	if ( ismarklike==true )
	    ap->type = at_centry;
	else
	    ap->type = at_cexit;
    } else if ( ismarklike==true )
	ap->type = at_mark;
    else
	ap->type = at_basechar;

    if ( a!=NULL ) {
	GTextInfo **ti = AnchorD_GlyphsInClass(a);
	GGadgetSetList(GWidgetGetControl(a->gw,CID_Glyph),ti,false);
	for ( i=0; ti[i]->text!=NULL || ti[i]->line; ++i ) {
	    if ( ti[i]->userdata == ap ) {
		GGadgetSelectOneListItem(GWidgetGetControl(a->gw,CID_Glyph),i);
	break;
	    }
	}
	AnchorD_ChangeGlyph(a,sc,ap);
    }
return( sc );
}

static int AnchorD_GlyphChanged(GGadget *g, GEvent *e) {
    AnchorDlg *a = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GTextInfo *sel = GGadgetGetListItemSelected(g);

	if ( sel!=NULL ) {
	    AnchorPoint *ap = sel->userdata;
	    if ( ap==Add_Mark )
		AddAnchor(a,a->sc->parent,a->ap->anchor,true);
	    else if ( ap==Add_Base )
		AddAnchor(a,a->sc->parent,a->ap->anchor,false);
	    else {
		char *name = u2utf8_copy(sel->text);
		SplineChar *sc = SFGetChar(a->sc->parent,-1,name);

		free(name);
		AnchorD_ChangeGlyph(a,sc,ap);
	    }
	}
    }
return( true );
}

static void AnchorD_NextPrev(AnchorDlg *a,int incr) {
    GGadget *g = GWidgetGetControl(a->gw,CID_Glyph);
    int len;
    GTextInfo **ti = GGadgetGetList(g,&len);
    int sel = GGadgetGetFirstListSelectedItem(g);

    for ( sel += incr; sel>0 && sel<len; sel+=incr ) {
	if ( !( ti[sel]->userdata == Add_Mark ||
		ti[sel]->userdata == Add_Base ||
		ti[sel]->line ||
		ti[sel]->disabled ))
    break;
    }
    if ( sel==0 || sel>=len )
	GDrawBeep(NULL);
    else {
	char *name = u2utf8_copy(ti[sel]->text);
	SplineChar *sc = SFGetChar(a->sc->parent,-1,name);

	free(name);
	GGadgetSelectOneListItem(g,sel);
	AnchorD_ChangeGlyph(a,sc,ti[sel]->userdata);
    }
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
	} else if ( event->u.chr.keysym == GK_Page_Down || event->u.chr.keysym == GK_KP_Page_Down ) {
	    AnchorD_NextPrev(a,1);
return( true );
	} else if ( event->u.chr.keysym == GK_Page_Up || event->u.chr.keysym == GK_KP_Page_Up ) {
	    AnchorD_NextPrev(a,-1);
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

static GTextInfo magnifications[] = {
    { (unichar_t *) "100%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "200%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "300%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "400%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
    
void AnchorControl(SplineChar *sc,AnchorPoint *ap,int layer) {
    GRect pos;
    GWindowAttrs wattrs;
    AnchorDlg a;
    GWindow gw;
    GGadgetCreateData gcd[15], buttonbox, maingcd[3], *hvarray[60], *buttonarray[8];
    GGadgetCreateData glyphbox, *glypharray[5];
    GTextInfo label[16];
    int k, hv;
    extern int _GScrollBar_Width;
    char buffer[20], xbuf[20], ybuf[20];
    GRect boxsize;

    memset(&a,0,sizeof(a));
    a.sc = sc;
    a.ap = ap;
    a.apos = ap->me;
    a.pixelsize = aa_pixelsize;
    a.magfactor = 1;
    a.layer = layer;
    if ( ap->xadjust.corrections!=NULL ) {
	int len = ap->xadjust.last_pixel_size-ap->xadjust.first_pixel_size+1;
	a.xadjust = ap->xadjust;
	a.xadjust.corrections = malloc(len);
	memcpy(a.xadjust.corrections,ap->xadjust.corrections,len);
    }
    if ( ap->yadjust.corrections!=NULL ) {
	int len = ap->yadjust.last_pixel_size-ap->yadjust.first_pixel_size+1;
	a.yadjust = ap->yadjust;
	a.yadjust.corrections = malloc(len);
	memcpy(a.yadjust.corrections,ap->yadjust.corrections,len);
    }

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
    if ( pos.height<aa_pixelsize+40+25 )
	pos.height = aa_pixelsize+40+25;
    pos.width -= 50;
    a.gw = gw = GDrawCreateTopWindow(NULL,&pos,anchord_e_h,&a,&wattrs);

    a.ctl_len = GDrawPointsToPixels(gw,140);
    a.sb_height = GDrawPointsToPixels(gw,_GScrollBar_Width);
    a.sb_base = pos.height - a.sb_height;

    memset(maingcd,0,sizeof(maingcd));
    memset(&buttonbox,0,sizeof(buttonbox));
    memset(&glyphbox,0,sizeof(glyphbox));
    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = hv = 0;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Glyph;
    gcd[k].gd.handle_controlevent = AnchorD_GlyphChanged;
    gcd[k++].creator = GListButtonCreate;

    glypharray[0] = GCD_Glue; glypharray[1] = &gcd[k-1]; glypharray[2] = GCD_Glue; glypharray[3] = NULL;

    glyphbox.gd.flags = gg_enabled|gg_visible;
    glyphbox.gd.u.boxelements = glypharray;
    glyphbox.creator = GHBoxCreate;

    hvarray[hv++] = &glyphbox; hvarray[hv++] = GCD_ColSpan; hvarray[hv++] = GCD_ColSpan;
    hvarray[hv++] = GCD_ColSpan; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;

    label[k].text = (unichar_t *) _("_Size:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 9;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup;
    gcd[k++].creator = GLabelCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan;

    sprintf( buffer, "%d", a.pixelsize );
    label[k].text = (unichar_t *) buffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 40; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_utf8_popup ;
    gcd[k].gd.cid = CID_DisplaySize;
    gcd[k].gd.handle_controlevent = AnchorD_DisplaySizeChanged;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg = (unichar_t *)
	    _("The size at which the current glyph is rasterized.\nFor small pixelsize you may want to use the magnification\nfactor below to get a clearer view.\n\nThe pulldown list contains the pixelsizes at which there\nare device table corrections.");
    gcd[k++].creator = GListFieldCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;

/* GT: Short for: Magnification */
    label[k].text = (unichar_t *) _("Mag:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+26;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup ;
    gcd[k++].creator = GLabelCreate;
    hvarray[hv++] = GCD_HPad10; hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan;

    gcd[k].gd.pos.x = 45; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup ;
    gcd[k].gd.cid = CID_Mag;
    gcd[k].gd.u.list = magnifications;
    gcd[k].gd.handle_controlevent = AnchorD_MagnificationChanged;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg = (unichar_t *)
	    _("The glyph is rasterized at the size above, but it\nmay be difficult to see the alignment errors\nthat can happen at small pixelsizes. This allows\nyou to expand each pixel to show potential problems\nbetter.");
    gcd[k++].creator = GListButtonCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;

    label[k].text = (unichar_t *) _("_X");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup ;
    gcd[k++].creator = GLabelCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan;

    sprintf( xbuf, "%d", (int) rint(ap->me.x) );
    label[k].text = (unichar_t *) xbuf;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 40; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup ;
    gcd[k].gd.cid = CID_X;
    gcd[k].gd.handle_controlevent = AnchorD_PositionChanged;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg = (unichar_t *)
	    _("The X coordinate of the anchor point in this glyph");
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;

/* GT: Short for Correction */
    label[k].text = (unichar_t *) _("Cor:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_utf8_popup ;
    gcd[k].gd.popup_msg = (unichar_t *) _("Correction in pixels to the horizontal positioning of this anchor point\nwhen rasterizing at the given pixelsize.\n(Lives in a Device Table)");
    gcd[k++].creator = GLabelCreate;
    hvarray[hv++] = GCD_HPad10; hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan;

    label[k].text = (unichar_t *) "0";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 45; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup ;
    gcd[k].gd.cid = CID_XCor;
    gcd[k].gd.handle_controlevent = AnchorD_CorrectionChanged;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg = (unichar_t *)
	    _("This is the number of pixels by which the anchor\nshould be moved horizontally when the glyph is\nrasterized at the above size.  This information\nis part of the device table for this anchor.\nDevice tables are particularly important at small\npixelsizes where rounding errors will have a\nproportionally greater effect.");
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;

    label[k].text = (unichar_t *) _("_Y");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup ;
    gcd[k++].creator = GLabelCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan;

    sprintf( ybuf, "%d", (int) rint(ap->me.y) );
    label[k].text = (unichar_t *) ybuf;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 40; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup ;
    gcd[k].gd.cid = CID_Y;
    gcd[k].gd.handle_controlevent = AnchorD_PositionChanged;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg = (unichar_t *)
	    _("The Y coordinate of the anchor point in this glyph");
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;

/* GT: Short for Correction */
    label[k].text = (unichar_t *) _("Cor:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_utf8_popup ;
    gcd[k].gd.popup_msg = (unichar_t *) _("Correction in pixels to the horizontal positioning of this anchor point\nwhen rasterizing at the given pixelsize.\n(Lives in a Device Table)");
    gcd[k++].creator = GLabelCreate;
    hvarray[hv++] = GCD_HPad10; hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_ColSpan;

    label[k].text = (unichar_t *) "0";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 45; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled  | gg_utf8_popup ;
    gcd[k].gd.cid = CID_YCor;
    gcd[k].gd.handle_controlevent = AnchorD_CorrectionChanged;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg = (unichar_t *)
	    _("This is the number of pixels by which the anchor\nshould be moved vertically when the glyph is\nrasterized at the above size.  This information\nis part of the device table for this anchor.\nDevice tables are particularly important at small\npixelsizes where rounding errors will have a\nproportionally greater effect.");
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;

    hvarray[hv++] = GCD_Glue; hvarray[hv++] = GCD_Glue; hvarray[hv++] = GCD_Glue;
    hvarray[hv++] = GCD_Glue; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+40;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_but_default;
    gcd[k].gd.handle_controlevent = AnchorD_OK;
    gcd[k++].creator = GButtonCreate;
    buttonarray[0] = GCD_Glue; buttonarray[1] = &gcd[k-1]; buttonarray[2] = GCD_Glue;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_but_cancel;
    gcd[k].gd.handle_controlevent = AnchorD_Cancel;
    gcd[k++].creator = GButtonCreate;
    buttonarray[3] = GCD_Glue; buttonarray[4] = &gcd[k-1]; buttonarray[5] = GCD_Glue;
    buttonarray[6] = NULL;

    buttonbox.gd.flags = gg_enabled|gg_visible;
    buttonbox.gd.u.boxelements = buttonarray;
    buttonbox.creator = GHBoxCreate;

    hvarray[hv++] = &buttonbox; hvarray[hv++] = GCD_ColSpan; hvarray[hv++] = GCD_ColSpan;
    hvarray[hv++] = GCD_ColSpan; hvarray[hv++] = GCD_Glue; hvarray[hv++] = NULL;
    hvarray[hv++] = NULL;

    maingcd[0].gd.pos.x = maingcd[0].gd.pos.y = 5;
    maingcd[0].gd.pos.height = pos.height - 10;
/*    maingcd[0].gd.pos.width = a.ctl_len - 10; */
    maingcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    maingcd[0].gd.u.boxelements = hvarray;
    maingcd[0].creator = GHVBoxCreate;

    maingcd[1].gd.pos.x = 300;
    maingcd[1].gd.pos.y = pos.height-a.sb_height;
    maingcd[1].gd.pos.height = a.sb_height;
    maingcd[1].gd.pos.width = pos.width-300;
    maingcd[1].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    maingcd[1].creator = GScrollBarCreate;

    GGadgetsCreate(a.gw,maingcd);
    GGadgetSetList(gcd[0].ret,AnchorD_GlyphsInClass(&a),false);
    GTextInfoListFree(gcd[0].gd.u.list);

    GHVBoxSetExpandableRow(maingcd[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(maingcd[0].ret,4);
    GHVBoxSetExpandableCol(buttonbox.ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(glyphbox.ret,gb_expandglue);
    GGadgetGetDesiredSize(maingcd[0].ret,&boxsize,NULL);
    a.ctl_len = boxsize.width + 10;
    
    a.hsb = maingcd[1].ret;

    AnchorD_FindComplements(&a);
    AnchorD_SetDevTabs(&a);
    AnchorD_ChangeSize(&a);
    AnchorD_SetTitle(&a);

    GDrawSetVisible(a.gw,true);
    while ( !a.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(a.gw);
    AnchorD_FreeAll(&a);
}

void AnchorControlClass(SplineFont *_sf,AnchorClass *ac,int layer) {
    /* Pick a random glyph with an anchor point in the class. If no glyph, */
    /*  give user the chance to create one */
    SplineChar *sc, *scmark = NULL;
    AnchorPoint *ap, *apmark = NULL;
    SplineFont *sf;
    int k, gid;

    if ( _sf->cidmaster!=NULL ) _sf = _sf->cidmaster;
    k=0;
    ap = NULL;
    do {
	sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
		if ( ap->anchor==ac ) {
		    if ( ap->type!=at_mark && ap->type!=at_centry )
	    break;
		    else if ( scmark==NULL ) {
			scmark = sc;
			apmark = ap;
		    }
		}
	    }
	    if ( ap!=NULL )
	break;
	}
	if ( ap!=NULL )
    break;
	++k;
    } while ( k<_sf->subfontcnt );

    if ( ap==NULL ) {
	sc = scmark;
	ap = apmark;
    }
    if ( ap==NULL ) {
	sc = AddAnchor(NULL,_sf,ac,-1);
	if ( sc==NULL )
return;
	for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	    if ( ap->anchor==ac )
	break;
	}
	if ( ap==NULL )		/* Can't happen */
return;
    }
	
    AnchorControl(sc,ap,layer);
}
