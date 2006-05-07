/* Copyright (C) 2000-2006 by George Williams */
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

static int mv_antialias = true;
static double mv_scales[] = { 2.0, 1.5, 1.0, 2.0/3.0, .5, 1.0/3.0, .25, .2, 1.0/6.0, .125, .1 };

static int MVSetVSb(MetricsView *mv);

static void MVDrawAnchorPoint(GWindow pixmap,MetricsView *mv,int i,struct aplist *apl) {
    SplineFont *sf = mv->fv->sf;
    int emsize = sf->ascent+sf->descent;
    double scale = mv->pixelsize / (double) emsize;
    double scaleas = mv->pixelsize / (double) (mv_scales[mv->scale_index]*emsize);
    AnchorPoint *ap = apl->ap;
    int x,y;

    if ( mv->bdf!=NULL )
return;

    y = mv->topend + 2 + scaleas * sf->ascent - mv->perchar[i].yoff - ap->me.y*scale - mv->yoff;
    x = mv->perchar[i].dx-mv->xoff+mv->perchar[i].xoff;
    if ( mv->perchar[i].selected )
	x += mv->activeoff;
    if ( mv->right_to_left )
	x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
    x += ap->me.x*scale;
    DrawAnchorPoint(pixmap,x,y,apl->selected);
}

static void MVVExpose(MetricsView *mv, GWindow pixmap, GEvent *event) {
    /* Expose routine for vertical metrics */
    GRect *clip, r, old2;
    int xbase, y, si, i, x, width, height;
    int as = rint(mv->pixelsize*mv->fv->sf->ascent/(double) (mv->fv->sf->ascent+mv->fv->sf->descent));
    BDFChar *bdfc;
    struct _GImage base;
    GImage gi;
    GClut clut;

    clip = &event->u.expose.rect;

    xbase = mv->dwidth/2;
    if ( mv->showgrid )
	GDrawDrawLine(pixmap,xbase,mv->topend,xbase,mv->displayend,0x808080);

    r.x = clip->x; r.width = clip->width;
    r.y = mv->topend; r.height = mv->displayend-mv->topend;
    GDrawPushClip(pixmap,&r,&old2);
    if ( mv->bdf==NULL && mv->showgrid ) {
	y = mv->perchar[0].dy-mv->yoff;
	GDrawDrawLine(pixmap,0,y,mv->dwidth,y,0x808080);
    }

    si = -1;
    for ( i=0; i<mv->charcnt; ++i ) {
	if ( mv->perchar[i].selected ) si = i;
	y = mv->perchar[i].dy-mv->yoff;
	if ( mv->bdf==NULL && mv->showgrid ) {
	    int yp = y+mv->perchar[i].dheight+mv->perchar[i].kernafter;
	    GDrawDrawLine(pixmap,0, yp,mv->dwidth,yp,0x808080);
	}
	y += mv->perchar[i].yoff;
	bdfc = mv->bdf==NULL ?	mv->perchar[i].show :
				mv->bdf->glyphs[mv->perchar[i].sc->orig_pos];
	if ( bdfc==NULL )
    continue;
	y += as-bdfc->ymax;
	if ( mv->perchar[i].selected )
	    y += mv->activeoff;
	x = xbase - mv->pixelsize/2 + bdfc->xmin - mv->perchar[i].xoff;
	width = bdfc->xmax-bdfc->xmin+1; height = bdfc->ymax-bdfc->ymin+1;
	if ( clip->y+clip->height<y )
    break;
	if ( y+height>=clip->y && x<clip->x+clip->width && x+width >= clip->x ) {
	    memset(&gi,'\0',sizeof(gi));
	    memset(&base,'\0',sizeof(base));
	    memset(&clut,'\0',sizeof(clut));
	    gi.u.image = &base;
	    base.clut = &clut;
	    if ( !bdfc->byte_data ) {
		base.image_type = it_mono;
		clut.clut_len = 2;
		clut.clut[0] = 0xffffff;
		if ( mv->perchar[i].selected )
		    clut.clut[1] = 0x808080;
	    } else {
		int scale = 3000/mv->pixelsize, l;
		Color fg, bg;
		if ( scale>4 ) scale = 4; else if ( scale==3 ) scale= 2;
		if ( mv->bdf!=NULL && mv->bdf->clut!=NULL )
		    scale = BDFDepth(mv->bdf);
		base.image_type = it_index;
		clut.clut_len = 1<<scale;
		bg = default_background;
		fg = ( mv->perchar[i].selected ) ? 0x808080 : 0x000000;
		for ( l=0; l<(1<<scale); ++l )
		    clut.clut[l] =
			COLOR_CREATE(
			 COLOR_RED(bg) + (l*(COLOR_RED(fg)-COLOR_RED(bg)))/((1<<scale)-1),
			 COLOR_GREEN(bg) + (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg)))/((1<<scale)-1),
			 COLOR_BLUE(bg) + (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg)))/((1<<scale)-1) );
	    }
	    base.data = bdfc->bitmap;
	    base.bytes_per_line = bdfc->bytes_per_line;
	    base.width = width;
	    base.height = height;
	    GDrawDrawImage(pixmap,&gi,NULL,x,y);
	}
    }
    if ( si!=-1 && mv->bdf==NULL && mv->showgrid ) {
	y = mv->perchar[si].dy-mv->yoff;
	if ( si!=0 )
	    GDrawDrawLine(pixmap,0,y,mv->dwidth,y,0x008000);
	y += mv->perchar[si].dheight+mv->perchar[si].kernafter;
	GDrawDrawLine(pixmap,0,y,mv->dwidth,y,0x000080);
    }
    GDrawPopClip(pixmap,&old2);
}

static void MVExpose(MetricsView *mv, GWindow pixmap, GEvent *event) {
    GRect old, *clip, r, old2;
    int x,y,ybase, width,height, i;
    SplineFont *sf = mv->fv->sf;
    BDFChar *bdfc;
    struct _GImage base;
    GImage gi;
    GClut clut;
    int si;
    int ke = mv->height-mv->sbh-(mv->fh+4);
    struct aplist *apl;
    double s = sin(-mv->fv->sf->italicangle*3.1415926535897932/180.);
    int x_iaoffh = 0, x_iaoffl = 0;

    clip = &event->u.expose.rect;
    if ( clip->y+clip->height < mv->topend )
return;
    GDrawPushClip(pixmap,clip,&old);
    GDrawSetLineWidth(pixmap,0);
    for ( x=mv->mwidth; x<mv->width; x+=mv->mwidth ) {
	GDrawDrawLine(pixmap,x,mv->displayend,x,ke,0x000000);
	GDrawDrawLine(pixmap,x+mv->mwidth/2,ke,x+mv->mwidth/2,mv->height-mv->sbh,0x000000);
    }
    GDrawDrawLine(pixmap,0,mv->topend,mv->width,mv->topend,0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend,mv->width,mv->displayend,0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+mv->fh+4,mv->width,mv->displayend+mv->fh+4,0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+2*(mv->fh+4),mv->width,mv->displayend+2*(mv->fh+4),0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+3*(mv->fh+4),mv->width,mv->displayend+3*(mv->fh+4),0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+4*(mv->fh+4),mv->width,mv->displayend+4*(mv->fh+4),0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+5*(mv->fh+4),mv->width,mv->displayend+5*(mv->fh+4),0x000000);
    if ( clip->y >= mv->displayend ) {
	GDrawPopClip(pixmap,&old);
return;
    }

    if ( mv->vertical ) {
	MVVExpose(mv,pixmap,event);
	GDrawPopClip(pixmap,&old);
return;
    }

    ybase = mv->topend + 2 + (mv->pixelsize/mv_scales[mv->scale_index] * sf->ascent / (sf->ascent+sf->descent)) - mv->yoff;
    if ( mv->showgrid )
	GDrawDrawLine(pixmap,0,ybase,mv->dwidth,ybase,0x808080);

    r.x = clip->x; r.width = clip->width;
    r.y = mv->topend; r.height = mv->displayend-mv->topend;
    GDrawPushClip(pixmap,&r,&old2);
    if ( mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[0].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->dwidth - x - mv->perchar[0].dwidth - mv->perchar[0].kernafter;
	GDrawDrawLine(pixmap,x,mv->topend,x,mv->displayend,0x808080);
	x_iaoffh = rint((ybase-mv->topend)*s), x_iaoffl = rint((mv->displayend-ybase)*s);
	if ( ItalicConstrained && x_iaoffh!=0 ) {
	    GDrawDrawLine(pixmap,x+x_iaoffh,mv->topend,x-x_iaoffl,mv->displayend,0x909090);
	}
    }
    si = -1;
    for ( i=0; i<mv->charcnt; ++i ) {
	if ( mv->perchar[i].selected ) si = i;
	x = mv->perchar[i].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
	if ( mv->bdf==NULL && mv->showgrid ) {
	    int xp = x+mv->perchar[i].dwidth+mv->perchar[i].kernafter;
	    GDrawDrawLine(pixmap,xp, mv->topend,xp,mv->displayend,0x808080);
	    if ( ItalicConstrained && x_iaoffh!=0 ) {
		GDrawDrawLine(pixmap,xp+x_iaoffh,mv->topend,xp-x_iaoffl,mv->displayend,0x909090);
	    }
	}
	if ( mv->right_to_left )
	    x += mv->perchar[i].kernafter-mv->perchar[i].xoff;
	else
	    x += mv->perchar[i].xoff;
	bdfc = mv->bdf==NULL ?	mv->perchar[i].show :
				mv->bdf->glyphs[mv->perchar[i].sc->orig_pos];
	if ( bdfc==NULL )
    continue;
	x += bdfc->xmin;
	if ( mv->perchar[i].selected )
	    x += mv->activeoff;
	y = ybase - bdfc->ymax - mv->perchar[i].yoff;
	width = bdfc->xmax-bdfc->xmin+1; height = bdfc->ymax-bdfc->ymin+1;
	if ( !mv->right_to_left && clip->x+clip->width<x )
    break;
	if ( x+width>=clip->x && y<clip->y+clip->height && y+height >= clip->y ) {
	    memset(&gi,'\0',sizeof(gi));
	    memset(&base,'\0',sizeof(base));
	    memset(&clut,'\0',sizeof(clut));
	    gi.u.image = &base;
	    base.clut = &clut;
	    if ( !bdfc->byte_data ) {
		base.image_type = it_mono;
		clut.clut_len = 2;
		clut.clut[0] = 0xffffff;
		if ( mv->perchar[i].selected )
		    clut.clut[1] = 0x808080;
	    } else {
		int lscale = 3000/mv->pixelsize, l;
		Color fg, bg;
		int scale;
		if ( lscale>4 ) lscale = 4; else if ( lscale==3 ) lscale= 2;
		if ( mv->bdf!=NULL && mv->bdf->clut!=NULL )
		    lscale = BDFDepth(mv->bdf);
		base.image_type = it_index;
		scale = lscale*lscale;
		clut.clut_len = scale;
		bg = default_background;
		fg = ( mv->perchar[i].selected ) ? 0x808080 : 0x000000;
		for ( l=0; l<scale; ++l )
		    clut.clut[l] =
			COLOR_CREATE(
			 COLOR_RED(bg) + ((int32) (l*(COLOR_RED(fg)-COLOR_RED(bg))))/(scale-1),
			 COLOR_GREEN(bg) + ((int32) (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg))))/(scale-1),
			 COLOR_BLUE(bg) + ((int32) (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg))))/(scale-1) );
	    }
	    base.data = bdfc->bitmap;
	    base.bytes_per_line = bdfc->bytes_per_line;
	    base.width = width;
	    base.height = height;
	    GDrawDrawImage(pixmap,&gi,NULL,x,y);
	}
	if ( mv->perchar[i].selected )
	    for ( apl=mv->perchar[i].aps; apl!=NULL; apl=apl->next )
		MVDrawAnchorPoint(pixmap,mv,i,apl);
    }
    if ( si!=-1 && mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[si].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->dwidth - x;
	if ( si!=0 )
	    GDrawDrawLine(pixmap,x,mv->topend,x,mv->displayend,0x008000);
	if ( mv->right_to_left )
	    x -= mv->perchar[si].dwidth+mv->perchar[si].kernafter;
	else
	    x += mv->perchar[si].dwidth+mv->perchar[si].kernafter;
	GDrawDrawLine(pixmap,x, mv->topend,x,mv->displayend,0x000080);
    }
    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old);
}

static void MVRedrawI(MetricsView *mv,int i,int oldxmin,int oldxmax) {
    GRect r;
    BDFChar *bdfc;
    int off = 0;

    if ( mv->right_to_left || mv->vertical ) {
	/* right to left clipping is hard to think about, it doesn't happen */
	/*  often enough (I think) for me to put the effort to make it efficient */
	GDrawRequestExpose(mv->gw,NULL,false);
return;
    }
    if ( mv->perchar[i].selected )
	off = mv->activeoff;
    r.y = mv->topend; r.height = mv->displayend-mv->topend;
    r.x = mv->perchar[i].dx-mv->xoff; r.width = mv->perchar[i].dwidth;
    if ( mv->perchar[i].kernafter>0 )
	r.width += mv->perchar[i].kernafter;
    if ( mv->perchar[i].hoff>0 )
	r.width += mv->perchar[i].hoff;
    if ( mv->perchar[i].xoff<0 ) {
	r.x += mv->perchar[i].xoff;
	r.width -= mv->perchar[i].xoff;
    } else
	r.width += mv->perchar[i].xoff;
    bdfc = mv->bdf==NULL ? mv->perchar[i].show :
			   mv->bdf->glyphs[mv->perchar[i].sc->orig_pos];
    if ( bdfc==NULL )
return;
    if ( bdfc->xmax+off+1>r.width ) r.width = bdfc->xmax+off+1;
    if ( oldxmax+1>r.width ) r.width = oldxmax+1;
    if ( bdfc->xmin+off<0 ) {
	r.x += bdfc->xmin+off;
	r.width -= bdfc->xmin+off;
    }
    if ( oldxmin<bdfc->xmin ) {
	r.width += (bdfc->xmin+off-oldxmin);
	r.x -= (bdfc->xmin+off-oldxmin);
    }
    if ( mv->right_to_left )
	r.x = mv->dwidth - r.x - r.width;
    GDrawRequestExpose(mv->gw,&r,false);
    if ( mv->perchar[i].selected && i!=0 ) {
	KernPair *kp; KernClass *kc; int index;
	for ( kp=mv->vertical ? mv->perchar[i-1].sc->vkerns: mv->perchar[i-1].sc->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc == mv->perchar[i].sc )
	break;
	if ( kp!=NULL ) {
	    mv->cur_sli = kp->sli;
	    if ( mv->sli_list!=NULL )
		GGadgetSelectOneListItem(mv->sli_list,kp->sli);
	} else if (( !mv->vertical &&
		    (kc=SFFindKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,false))!=NULL ) ||
		( mv->vertical &&
		    (kc=SFFindVKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,false))!=NULL )) {
	    mv->cur_sli = kc->sli;
	    if ( mv->sli_list!=NULL )
		GGadgetSelectOneListItem(mv->sli_list,kc->sli);
	}
    }
}

static void MVDeselectChar(MetricsView *mv, int i) {

    mv->perchar[i].selected = false;
    if ( mv->perchar[i].name!=NULL )
	GGadgetSetEnabled(mv->perchar[i].name,mv->bdf==NULL);
    MVRedrawI(mv,i,0,0);
#if 0
    if ( mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[i].dx;
	if ( mv->right_to_left )
	    x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
	GDrawDrawLine(mv->gw,x,mv->topend,x,mv->displayend,0x808080);
	x += mv->perchar[i].dwidth+mv->perchar[i].kernafter;
	GDrawDrawLine(mv->gw,x,mv->topend,x,mv->displayend,0x808080);
    }
#endif
}

static void MVSelectChar(MetricsView *mv, int i) {

    mv->perchar[i].selected = true;
    if ( mv->perchar[i].name!=NULL )
	GGadgetSetEnabled(mv->perchar[i].name,false);
    if ( mv->sli_list!=NULL && SCScriptInSLI(mv->fv->sf,mv->perchar[i].sc,mv->cur_sli)) {
	/* The script for the current character is in the current sli */
	/*  use it without change */
    } else if ( mv->sli_list!=NULL && SCScriptFromUnicode(mv->perchar[i].sc)!=DEFAULT_SCRIPT ) {
	int sli_cnt = SLICount(mv->fv->sf);
	int sli = SCDefaultSLI(mv->fv->sf,mv->perchar[i].sc);
	mv->cur_sli = sli;
	if ( SLICount(mv->fv->sf)!=sli_cnt )
	    GGadgetSetList(mv->sli_list,SFLangArray(mv->fv->sf,true),false);
	GGadgetSelectOneListItem(mv->sli_list,sli);
    }
    MVRedrawI(mv,i,0,0);
#if 0
    if ( mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[i].dx;
	if ( x==0 )		/* Not set properly yet */
return;
	if ( mv->right_to_left )
	    x = mv->dwidth - x;
	if ( i!=0 )
	    GDrawDrawLine(mv->gw,x,mv->topend,x,mv->displayend,0x008000);
	if ( mv->right_to_left )
	    x -= mv->perchar[i].dwidth+mv->perchar[i].kernafter;
	else
	    x += mv->perchar[i].dwidth+mv->perchar[i].kernafter;
	GDrawDrawLine(mv->gw,x,mv->topend,x,mv->displayend,0x000080);
    }
#endif
}
    
static void MVDoSelect(MetricsView *mv, int i) {
    int j;

    for ( j=0; j<mv->charcnt; ++j )
	if ( j!=i && mv->perchar[j].selected )
	    MVDeselectChar(mv,j);
    MVSelectChar(mv,i);
}

void MVRefreshChar(MetricsView *mv, SplineChar *sc) {
    int i;
    for ( i=0; i<mv->charcnt; ++i ) if ( mv->perchar[i].sc == sc )
	MVRedrawI(mv,i,0,0);
}

static void MVRefreshKern(MetricsView *mv, int i) {
    /* We need to look through the kern pairs at sc[i-1] to see if the is a */
    /*  match for sc[i] */
    KernPair *kp;
    KernClass *kc=NULL;
    int index, sli, off;
    unichar_t ubuf[40];
    char buf[40];

    if ( mv->perchar[i].kern==NULL )	/* Happens during init, we'll do it later */
return;
    for ( kp=mv->vertical ? mv->perchar[i-1].sc->vkerns:mv->perchar[i-1].sc->kerns; kp!=NULL; kp=kp->next )
	if ( kp->sc == mv->perchar[i].sc )
    break;
    sli = -1; off = 0;
    if ( kp!=NULL && kp->off != 0 ) {
	off = kp->off;
	sli = kp->sli;
    } else if (( !mv->vertical &&
		(kc=SFFindKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,false))!=NULL ) ||
	    ( mv->vertical &&
		(kc=SFFindVKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,false))!=NULL )) {
	off = kc->offsets[index];
	sli = kc->sli;
    } else if ( kp!=NULL )
	sli = kp->sli;
    if ( off==0 )
	buf[0] = '\0';
    else
	sprintf(buf,"%d",off);
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(mv->perchar[i].kern,ubuf);
    mv->perchar[i-1].kernafter = off * mv->pixelsize/
	    (mv->fv->sf->ascent+mv->fv->sf->descent);
    if ( sli!=-1 ) {
	mv->cur_sli = sli;
	GGadgetSelectOneListItem(mv->sli_list,sli);
    }
}

static void aplistfree(struct aplist *l) {
    struct aplist *next;

    for ( ; l!=NULL; l=next ) {
	next = l->next;
	chunkfree(l,sizeof(*l));
    }
}

static struct aplist *aplistcreate(struct aplist *n,AnchorPoint *ap, int connect) {
    struct aplist *l = chunkalloc(sizeof(*n));

    l->next = n;
    l->ap = ap;
    l->connected_to = connect;
return( l );
}

static int MVSetAnchor(MetricsView *mv) {
    int i, j, changed=false;
    AnchorPos *apos, *test;
    int base, newx, newy;
    int emsize = mv->fv->sf->ascent+mv->fv->sf->descent;

    for ( i=0; i<mv->charcnt; ++i ) {
	mv->sstr[i] = mv->perchar[i].sc;
	aplistfree(mv->perchar[i].aps);
	mv->perchar[i].aps = NULL;
	if ( mv->perchar[i].xoff!=0 || mv->perchar[i].yoff!=0 ||
		mv->perchar[i].hoff!=0 || mv->perchar[i].voff!=0 ) {
	    mv->perchar[i].xoff = mv->perchar[i].yoff = 0;
	    mv->perchar[i].hoff = mv->perchar[i].voff = 0;
	    changed = true;
	}
    }
    if ( !mv->vertical ) {
	mv->sstr[i] = NULL;
	for ( i=0; i<mv->charcnt; ++i ) {
	    apos = AnchorPositioning(mv->sstr[i],NULL,mv->sstr+i+1);
	    base = i;
	    if ( apos!=NULL ) {
		for ( test=apos; test->sc!=NULL ; ++test ) {
		    ++i;
		    j = base+1+apos->base_index;
		    newx = mv->perchar[j].xoff - (mv->perchar[i].dx-mv->perchar[j].dx) +
			    test->x * mv->pixelsize / emsize;
		    if ( mv->right_to_left )
			newx = -mv->perchar[j].sc->width * mv->pixelsize/emsize - newx;
		    newy = mv->perchar[j].yoff + test->y * mv->pixelsize / emsize;
		    if ( mv->perchar[i].xoff != newx || mv->perchar[i].yoff != newy ) {
			mv->perchar[i].xoff = newx; mv->perchar[i].yoff = newy;
			changed = true;
		    }
		    mv->perchar[i].aps = aplistcreate(mv->perchar[i].aps,test->apm,j);
		    mv->perchar[j].aps = aplistcreate(mv->perchar[j].aps,test->apb,i);
		}
		if ( i<mv->charcnt ) {
		    if ( mv->perchar[i+1].xoff!=0 || mv->perchar[i+1].yoff!=0 ) {
			mv->perchar[i+1].xoff = mv->perchar[i+1].yoff = 0;
			changed = true;
		    }
		}
		    
		if ( test[-1].apm->type==at_centry )
		    --i;
		AnchorPosFree(apos);
	    }
	}
    }

    for ( i=0; i<mv->max; ++i ) if ( mv->perchar[i].width!=NULL ) {
	if ( mv->perchar[i].active_pos!=NULL ) {
	    mv->perchar[i].xoff += mv->perchar[i].active_pos->u.pos.xoff*mv->pixelsize/ emsize;
	    mv->perchar[i].yoff += mv->perchar[i].active_pos->u.pos.yoff*mv->pixelsize/ emsize;
	    mv->perchar[i].hoff += mv->perchar[i].active_pos->u.pos.h_adv_off*mv->pixelsize/ emsize;
	    mv->perchar[i].voff += mv->perchar[i].active_pos->u.pos.v_adv_off*mv->pixelsize/ emsize;
	}
    }
return( changed );
}

static void MVRefreshValues(MetricsView *mv, int i, SplineChar *sc) {
    unichar_t ubuf[40];
    char buf[40];
    DBounds bb;

    SplineCharFindBounds(sc,&bb);

    uc_strcpy(ubuf,sc->name);
    GGadgetSetTitle(mv->perchar[i].name,ubuf);

    sprintf(buf,"%d",mv->vertical ? sc->vwidth : sc->width);
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(mv->perchar[i].width,ubuf);

    sprintf(buf,"%.2f",mv->vertical ? sc->parent->ascent-bb.maxy : bb.minx);
    if ( buf[strlen(buf)-1]=='0' ) {
	buf[strlen(buf)-1] = '\0';
	if ( buf[strlen(buf)-1]=='0' ) {
	    buf[strlen(buf)-1] = '\0';
	    if ( buf[strlen(buf)-1]=='.' )
		buf[strlen(buf)-1] = '\0';
	}
    }
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(mv->perchar[i].lbearing,ubuf);

    sprintf(buf,"%.2f",mv->vertical ? sc->vwidth-(sc->parent->ascent-bb.miny) : sc->width-bb.maxx);
    if ( buf[strlen(buf)-1]=='0' ) {
	buf[strlen(buf)-1] = '\0';
	if ( buf[strlen(buf)-1]=='0' ) {
	    buf[strlen(buf)-1] = '\0';
	    if ( buf[strlen(buf)-1]=='.' )
		buf[strlen(buf)-1] = '\0';
	}
    }
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(mv->perchar[i].rbearing,ubuf);

    if ( i!=0 )
	MVRefreshKern(mv,i);
    if ( i!=mv->charcnt-1 )
	MVRefreshKern(mv,i+1);
    else
	mv->perchar[i].kernafter = 0;
}

void MVReKern(MetricsView *mv) {
    int i;
    for ( i=1; i<mv->charcnt; ++i )
	MVRefreshKern(mv,i);
    mv->perchar[mv->charcnt-1].kernafter = 0;
    for ( i=1; i<mv->charcnt; ++i )
	mv->perchar[i].dx = mv->perchar[i-1].dx+mv->perchar[i-1].dwidth+
		mv->perchar[i-1].kernafter;
    MVSetAnchor(mv);
    GDrawRequestExpose(mv->gw,NULL,false);
}

static BDFChar *MVRasterize(MetricsView *mv,SplineChar *sc) {
    BDFChar *bdfc=NULL;

    bdfc = SplineCharFreeTypeRasterizeNoHints(sc,mv->pixelsize,mv->antialias?4:1);
    if ( bdfc!=NULL )
	/* All done */;
    else if ( mv->antialias && mv->pixelsize<1000 ) {
	int scale = 3000/mv->pixelsize;
	if ( scale>4 ) scale=4;
	if ( scale==3 ) scale = 2;
	bdfc = SplineCharAntiAlias(sc,mv->pixelsize,scale);
    } else
	bdfc = SplineCharRasterize(sc,mv->pixelsize);
return( bdfc );
}

static void MVReRasterize(MetricsView *mv) {
    int i;
    double scale = mv->pixelsize/(double) (mv->fv->sf->ascent+mv->fv->sf->descent);

    if ( mv->bdf==NULL ) {
	for ( i=0; i<mv->charcnt; ++i ) {
	    BDFChar *bc = MVRasterize(mv,mv->perchar[i].sc);
	    BDFCharFree(mv->perchar[i].show);
	    mv->perchar[i].show = bc;
	    mv->perchar[i].dwidth = bc->width;
	    if ( mv->vertical )
		mv->perchar[i].dheight = rint(bc->sc->vwidth*scale);
	    if ( i+1<mv->charcnt ) {
		mv->perchar[i+1].dx = mv->perchar[i].dx + bc->width + mv->perchar[i].kernafter;
		if ( mv->vertical )
		    mv->perchar[i+1].dy = mv->perchar[i].dy + mv->perchar[i].dheight +
			    mv->perchar[i].kernafter;
	    }
	}
    }
}

void MVRegenChar(MetricsView *mv, SplineChar *sc) {
    int i;
    BDFChar *bdfc;
    int xoff = 0, yoff=0, vwidth;
    int oldxmax,oldxmin;

    for ( i=0; i<mv->charcnt; ++i ) {
	mv->perchar[i].dx += xoff;
	if ( mv->perchar[i].sc == sc ) {
	    if ( mv->bdf==NULL ) {
		bdfc = MVRasterize(mv,sc);
		oldxmax = mv->perchar[i].show->xmax;
		oldxmin = mv->perchar[i].show->xmin;
		BDFCharFree(mv->perchar[i].show);
		mv->perchar[i].show = bdfc;
		xoff += (bdfc->width-mv->perchar[i].dwidth);
		mv->perchar[i].dwidth = bdfc->width;
		if ( !mv->vertical ) {
		    if ( xoff==0 )
			MVRedrawI(mv,i,oldxmin,oldxmax);
		} else {
		    vwidth = rint( sc->vwidth * mv->pixelsize/(double) (sc->parent->ascent+sc->parent->descent));
		    yoff += vwidth - mv->perchar[i].dheight;
		    mv->perchar[i].dheight = vwidth;
		}
	    }
	    MVRefreshValues(mv,i,sc);
	}
    }
    if ( MVSetAnchor(mv) || xoff!=0 || mv->vertical )
	GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVChangeDisplayFont(MetricsView *mv, BDFFont *bdf) {
    int i;
    BDFChar *bc;
    double scale = mv->pixelsize/(double) (mv->fv->sf->ascent+mv->fv->sf->descent);

    if ( mv->bdf==bdf )
return;
    if ( (mv->bdf==NULL) != (bdf==NULL) ) {
	for ( i=0; i<mv->max; ++i ) if ( mv->perchar[i].width!=NULL ) {
	    GGadgetSetEnabled(mv->perchar[i].width,bdf==NULL);
	    GGadgetSetEnabled(mv->perchar[i].lbearing,bdf==NULL);
	    GGadgetSetEnabled(mv->perchar[i].rbearing,bdf==NULL);
	    if ( i!=0 )
		GGadgetSetEnabled(mv->perchar[i].kern,bdf==NULL);
	}
    }
    if ( mv->bdf==NULL ) {
	for ( i=0; i<mv->charcnt; ++i ) {
	    BDFCharFree(mv->perchar[i].show);
	    mv->perchar[i].show = NULL;
	}
    } else if ( bdf==NULL ) {
	for ( i=0; i<mv->charcnt; ++i ) {
	    mv->perchar[i].show = MVRasterize(mv,mv->perchar[i].sc);
	}
    }
    mv->bdf = bdf;
    for ( i=0; i<mv->charcnt; ++i ) {
	bc = bdf==NULL?mv->perchar[i].show:bdf->glyphs[mv->perchar[i].sc->orig_pos];
	mv->perchar[i].dwidth = bc->width;
	if ( i+1<mv->charcnt )
	    mv->perchar[i+1].dx = mv->perchar[i].dx+bc->width+mv->perchar[i].kernafter;
	if ( mv->vertical ) {
	    mv->perchar[i].dheight = rint(bc->sc->vwidth*scale);
	    if ( i+1<mv->charcnt )
		mv->perchar[i+1].dy = mv->perchar[i].dy+mv->perchar[i].dheight+
			mv->perchar[i].kernafter;
	}
    }
}

static void setlabel(GGadget *lab, char *text) {
    unichar_t ubuf[30];

    uc_strncpy(ubuf,text,sizeof(ubuf)/sizeof(ubuf[0]));
    GGadgetSetTitle(lab,ubuf);
}

static int MV_WidthChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->perchar[which].sc;
	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && val!=sc->width ) {
	    SCPreserveWidth(sc);
	    sc->width = val;
	    SCCharChangedUpdate(sc);
	} else if ( mv->vertical && val!=sc->vwidth ) {
	    SCPreserveVWidth(sc);
	    sc->vwidth = val;
	    SCCharChangedUpdate(sc);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->charcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int MV_LBearingChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->perchar[which].sc;
	DBounds bb;
	SplineCharFindBounds(sc,&bb);
	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && val!=bb.minx ) {
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[5] = 0;
	    transform[4] = val-bb.minx;
	    FVTrans(mv->fv,sc,transform,NULL,false);
	} else if ( mv->vertical && val!=sc->parent->ascent-bb.maxy ) {
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[4] = 0;
	    transform[5] = sc->parent->ascent-bb.maxy-val;
	    FVTrans(mv->fv,sc,transform,NULL,false);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->charcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int MV_RBearingChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->perchar[which].sc;
	DBounds bb;
	SplineCharFindBounds(sc,&bb);
	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && val!=sc->width-bb.maxx ) {
	    SCPreserveWidth(sc);
	    sc->width = rint(bb.maxx+val);
	    /* Width is an integer. Adjust the lbearing so that the rbearing */
	    /*  remains what was just typed in */
	    if ( sc->width!=bb.maxx+val ) {
		real transform[6];
		transform[0] = transform[3] = 1.0;
		transform[1] = transform[2] = transform[5] = 0;
		transform[4] = sc->width-val-bb.maxx;
		FVTrans(mv->fv,sc,transform,NULL,false);
	    }
	    SCCharChangedUpdate(sc);
	} else if ( mv->vertical && val!=sc->vwidth-(sc->parent->ascent-bb.miny) ) {
	    double vw = val+(sc->parent->ascent-bb.miny);
	    SCPreserveWidth(sc);
	    sc->vwidth = rint(vw);
	    /* Width is an integer. Adjust the lbearing so that the rbearing */
	    /*  remains what was just typed in */
	    if ( sc->width!=vw ) {
		real transform[6];
		transform[0] = transform[3] = 1.0;
		transform[1] = transform[2] = transform[4] = 0;
		transform[5] = vw-sc->vwidth;
		FVTrans(mv->fv,sc,transform,NULL,false);
	    }
	    SCCharChangedUpdate(sc);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->charcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int AskNewKernClassEntry(SplineChar *fsc,SplineChar *lsc) {
    char *yesno[3];
#if defined(FONTFORGE_CONFIG_GDRAW)
    yesno[0] = _("_Yes");
    yesno[1] = _("_No");
#elif defined(FONTFORGE_CONFIG_GTK)
    yesno[0] = GTK_STOCK_YES;
    yesno[1] = GTK_STOCK_NO;
#endif
    yesno[2] = NULL;
return( gwwv_ask(_("Use Kerning Class?"),(const char **) yesno,0,1,_("This kerning pair (%.20s and %.20s) is currently part of a kerning class with a 0 offset for this combination. Would you like to alter this kerning class entry (or create a kerning pair for just these two glyphs)?"),
	fsc->name,lsc->name)==0 );
}

static int MV_KernChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->perchar[which].sc;
	SplineChar *psc = mv->perchar[which-1].sc;
	static unichar_t zerostr[] = { /*'0',*/  '\0' };
	KernPair *kp;
	KernClass *kc; int index;
	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else {
	    kc = NULL;
	    for ( kp = mv->vertical ? psc->vkerns : psc->kerns; kp!=NULL && kp->sc!=sc; kp = kp->next );
	    if ( kp==NULL )
		kc= !mv->vertical
		    ? SFFindKernClass(mv->fv->sf,psc,sc,&index,true)
		    : SFFindVKernClass(mv->fv->sf,psc,sc,&index,true);
	    if ( kc!=NULL ) {
		if ( kc->offsets[index]==0 && !AskNewKernClassEntry(psc,sc))
		    kc=NULL;
		else
		    kc->offsets[index] = val;
	    }
	    if ( kc==NULL ) {
		if ( !mv->vertical )
		    MMKern(sc->parent,psc,sc,kp==NULL?val:val-kp->off,
			    mv->cur_sli,kp);
		if ( kp==NULL ) {
		    kp = chunkalloc(sizeof(KernPair));
		    kp->sc = sc;
		    if ( !mv->vertical ) {
			kp->next = psc->kerns;
			psc->kerns = kp;
		    } else {
			kp->next = psc->vkerns;
			psc->vkerns = kp;
		    }
		}
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		/* If we change the kerning offset, then any pixel corrections*/
		/*  will no longer apply (they only had meaning with the old  */
		/*  offset) so free the device table, if any */
		if ( kp->off!=val ) {
		    DeviceTableFree(kp->adjust);
		    kp->adjust = NULL;
		}
#endif
		kp->off = val;
		kp->sli = mv->cur_sli;
	    }
	    mv->perchar[which-1].kernafter = (val*mv->pixelsize)/
		    (mv->fv->sf->ascent+mv->fv->sf->descent);
	    for ( i=which; i<mv->charcnt; ++i ) {
		mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth + mv->perchar[i-1].kernafter;
		if ( mv->vertical )
		    mv->perchar[i].dy = mv->perchar[i-1].dy + mv->perchar[i-1].dheight + mv->perchar[i-1].kernafter;
	    }
	    mv->fv->sf->changed = true;
	    /* I don't bother remove kernpairs that go to zero here, there's */
	    /*  little point, it's just easier not to save them out */
	    if ( *_GGadgetGetTitle(g)=='\0' )
		GGadgetSetTitle(g,zerostr);
	    GDrawRequestExpose(mv->gw,NULL,false);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->charcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static void MVMakeLabels(MetricsView *mv) {
    static GBox small = { 0 };
    GGadgetData gd;
    GTextInfo label;

    small.main_background = small.main_foreground = COLOR_DEFAULT;
    memset(&gd,'\0',sizeof(gd));
    memset(&label,'\0',sizeof(label));

    mv->mwidth = GGadgetScale(60);
    mv->displayend = mv->height- mv->sbh - 5*(mv->fh+4);
    mv->pixelsize = mv_scales[mv->scale_index]*(mv->displayend - mv->topend - 4);

    label.text = (unichar_t *) _("Name:");
    label.text_is_1byte = true;
    label.font = mv->font;
    gd.pos.x = 2; gd.pos.width = mv->mwidth-4;
    gd.pos.y = mv->displayend+2;
    gd.pos.height = mv->fh;
    gd.label = &label;
    gd.box = &small;
    gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_dontcopybox;
    mv->namelab = GLabelCreate(mv->gw,&gd,NULL);

    label.text = (unichar_t *) (mv->vertical ? _("Height:") : _("Width:") );
    gd.pos.y += mv->fh+4;
    mv->widthlab = GLabelCreate(mv->gw,&gd,NULL);

/* GT: Top/Left (side) bearing */
    label.text = (unichar_t *) (mv->vertical ? _("TBearing:") : _("LBearing:") );
    gd.pos.y += mv->fh+4;
    mv->lbearinglab = GLabelCreate(mv->gw,&gd,NULL);

/* GT: Bottom/Right (side) bearing */
    label.text = (unichar_t *) (mv->vertical ? _("BBearing:") : _("RBearing:") );
    gd.pos.y += mv->fh+4;
    mv->rbearinglab = GLabelCreate(mv->gw,&gd,NULL);

    label.text = (unichar_t *) (mv->vertical ? _("VKern:") : _("Kern:"));
    gd.pos.y += mv->fh+4;
    mv->kernlab = GLabelCreate(mv->gw,&gd,NULL);
}

static void MVCreateFields(MetricsView *mv,int i) {
    static GBox small = { 0 };
    GGadgetData gd;
    GTextInfo label;
    static unichar_t nullstr[1] = { 0 };
    int j;

    small.main_background = small.main_foreground = COLOR_DEFAULT;
    small.disabled_foreground = 0x808080;
    small.disabled_background = COLOR_DEFAULT;

    memset(&gd,'\0',sizeof(gd));
    memset(&label,'\0',sizeof(label));
    label.text = nullstr;
    label.font = mv->font;
    mv->perchar[i].mx = gd.pos.x = mv->mbase+(i+1-mv->coff)*mv->mwidth+2;
    mv->perchar[i].mwidth = gd.pos.width = mv->mwidth-4;
    gd.pos.y = mv->displayend+2;
    gd.pos.height = mv->fh;
    gd.label = &label;
    gd.box = &small;
    gd.flags = gg_visible | gg_pos_in_pixels | gg_dontcopybox;
    if ( mv->bdf==NULL )
	gd.flags |= gg_enabled;
    mv->perchar[i].name = GLabelCreate(mv->gw,&gd,(void *) (intpt) i);
    if ( mv->perchar[i].selected )
	GGadgetSetEnabled(mv->perchar[i].name,false);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_WidthChanged;
    mv->perchar[i].width = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_LBearingChanged;
    mv->perchar[i].lbearing = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_RBearingChanged;
    mv->perchar[i].rbearing = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);

    if ( i!=0 ) {
	gd.pos.y += mv->fh+4;
	gd.pos.x -= mv->mwidth/2;
	gd.handle_controlevent = MV_KernChanged;
	mv->perchar[i].kern = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);

	if ( i>=mv->charcnt ) {
	    for ( j=mv->charcnt+1; j<=i ; ++ j )
		mv->perchar[j].dx = mv->perchar[j-1].dx;
	    mv->charcnt = i+1;
	}
    }

    GWidgetIndicateFocusGadget(mv->text);
}

static void MVSetPos(MetricsView *mv,int i,SplineChar *sc) {
    BDFChar *bdfc;
    int j;

    if ( i>=mv->max ) {
	int oldmax=mv->max;
	mv->max = i+10;
	mv->perchar = grealloc(mv->perchar,mv->max*sizeof(struct metricchar));
	mv->sstr = grealloc(mv->sstr,(mv->max+1)*sizeof(SplineChar *));
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }
    mv->perchar[i].sc = sc;
    mv->perchar[i].basesc = NULL;
    mv->perchar[i].active_pos = NULL;
    if ( mv->bdf==NULL ) {
	bdfc = MVRasterize(mv,sc);
	BDFCharFree(mv->perchar[i].show);
	mv->perchar[i].show = bdfc;
    } else
	bdfc = mv->bdf->glyphs[mv->perchar[i].sc->orig_pos];
    mv->perchar[i].dwidth = bdfc->width;
    if ( mv->vertical )
	mv->perchar[i].dheight = rint(bdfc->sc->vwidth*mv->pixelsize/(double) (mv->fv->sf->ascent+mv->fv->sf->descent));
    if ( mv->perchar[i].width==NULL ) {
	mv->perchar[i].dx = 0;		/* Flag => Don't draw when focus changes (focus changes as we create text fields) */
	MVCreateFields(mv,i);
    }
    if ( i>=mv->charcnt ) mv->charcnt = i+1;
    MVRefreshValues(mv,i,sc);
    if ( i==0 ) {
	mv->perchar[i].dx = 10;
	mv->perchar[i].dy = mv->topend + 10;
	j=i+1;
    } else {
	mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth + mv->perchar[i-1].kernafter;
	j = i;
    }
    for ( ; j<mv->charcnt; ++j )
	mv->perchar[j].dx = mv->perchar[j-1].dx + mv->perchar[j-1].dwidth + mv->perchar[j-1].kernafter;
    if ( mv->vertical ) {
	if ( i==0 ) {
	    mv->perchar[i].dy = mv->topend + 10;
	    j=i+1;
	} else {
	    mv->perchar[i].dy = mv->perchar[i-1].dy + mv->perchar[i-1].dheight + mv->perchar[i-1].kernafter;
	    j = i;
	}
	for ( ; j<mv->charcnt; ++j )
	    mv->perchar[j].dy = mv->perchar[j-1].dy + mv->perchar[j-1].dheight + mv->perchar[j-1].kernafter;
    }
    MVSetVSb(mv);
}

static void MVToggleVertical(MetricsView *mv) {
    int size,i;

    mv->vertical = !mv->vertical;

    setlabel( mv->widthlab, mv->vertical ? "Height:" : "Width:" );
    setlabel( mv->lbearinglab, mv->vertical ? "TBearing:" : "LBearing:" );
    setlabel( mv->rbearinglab, mv->vertical ? "BBearing:" : "RBearing:" );
    setlabel( mv->kernlab, mv->vertical ? "VKern:" : "Kern:" );

    if ( mv->vertical )
	if ( mv->scale_index<4 ) mv->scale_index = 4;

    size = (mv->displayend - mv->topend - 4);
    if ( mv->dwidth-20<size )
	size = mv->dwidth-20;
    size *= mv_scales[mv->scale_index];
    if ( mv->pixelsize != size ) {
	mv->pixelsize = size;
	MVReRasterize(mv);
    }
    for ( i=0; i<mv->charcnt; ++i ) {
	MVSetPos(mv,i,mv->perchar[i].sc);
	MVRefreshValues(mv,i,mv->perchar[i].sc);
    }
    MVReKern(mv);
    MVSetAnchor(mv);
}

static SplineChar *SCFromUnicode(SplineFont *sf, EncMap *map, int ch) {
    int i = SFFindSlot(sf,map,ch,NULL);

    if ( i==-1 )
return( NULL );
    else
return( SFMakeChar(sf,map,i) );
}

static void MVMoveFieldsBy(MetricsView *mv,int diff) {
    int i;
    int y,x;

    for ( i=0; i<mv->max && mv->perchar[i].width!=NULL; ++i ) {
	y = mv->displayend+2;
	x = mv->perchar[i].mx-diff;
	if ( x<mv->mbase+mv->mwidth ) x = -2*mv->mwidth;
	GGadgetMove(mv->perchar[i].name,x,y);
	y += mv->fh+4;
	GGadgetMove(mv->perchar[i].width,x,y);
	y += mv->fh+4;
	GGadgetMove(mv->perchar[i].lbearing,x,y);
	y += mv->fh+4;
	GGadgetMove(mv->perchar[i].rbearing,x,y);
	y += mv->fh+4;
	if ( i!=0 )
	    GGadgetMove(mv->perchar[i].kern,x-mv->mwidth/2,y);
    }
}

static int MVDisplayedCnt(MetricsView *mv) {
    int i, wid = mv->mbase;

    for ( i=mv->coff; i<mv->charcnt; ++i ) {
	wid += mv->perchar[i].dwidth;
	if ( wid>mv->dwidth )
return( i-mv->coff );
    }
return( i-mv->coff );		/* There's extra room. don't know exactly how much but allow for some */
}

static void MVSetSb(MetricsView *mv) {
    int cnt = (mv->dwidth-mv->mbase-mv->mwidth)/mv->mwidth;
    int dcnt = MVDisplayedCnt(mv);

    if ( cnt>dcnt ) cnt = dcnt;
    if ( cnt==0 ) cnt = 1;

    GScrollBarSetBounds(mv->hsb,0,mv->charcnt,cnt);
    GScrollBarSetPos(mv->hsb,mv->coff);
#if 0
    /* No, the scroll bar will go the same way, just as the fields do. */
    /* the text will scroll reverse. Confusing as hell */
    if ( !mv->right_to_left ) {
    } else {
	GScrollBarSetBounds(mv->hsb,-mv->charcnt,0,cnt);
	GScrollBarSetPos(mv->hsb,-mv->coff);
    }
#endif
}

static int MVSetVSb(MetricsView *mv) {
    int max, min, i, ret, ybase, yoff;

    if ( mv->vertical ) {
	min = max = 0;
	if ( mv->charcnt!=0 )
	    max = mv->perchar[mv->charcnt-1].dy + mv->perchar[mv->charcnt-1].dheight;
    } else {
	SplineFont *sf = mv->fv->sf;
	ybase = 2 + (mv->pixelsize/mv_scales[mv->scale_index] * sf->ascent / (sf->ascent+sf->descent));
	min = -ybase;
	max = mv->displayend-mv->topend-ybase;
	for ( i=0; i<mv->charcnt; ++i ) {
	    BDFChar *bdfc = mv->bdf==NULL ? mv->perchar[i].show : mv->bdf->glyphs[mv->perchar[i].sc->orig_pos];
	    if ( bdfc!=NULL ) {
		if ( min>-bdfc->ymax ) min = -bdfc->ymax;
		if ( max<-bdfc->ymin ) max = -bdfc->ymin;
	    }
	}
	min += ybase;
	max += ybase;
    }
    min -= 10;
    max += 10;
    GScrollBarSetBounds(mv->vsb,min,max,mv->displayend-mv->topend);
    yoff = mv->yoff;
    if ( yoff+mv->displayend-mv->topend > max )
	yoff = max - (mv->displayend-mv->topend);
    if ( yoff<min ) yoff = min;
    ret = yoff!=mv->yoff;
    mv->yoff = yoff;
    GScrollBarSetPos(mv->vsb,yoff);
return( ret );
}

static void MVHScroll(MetricsView *mv,struct sbevent *sb) {
    int newpos = mv->coff;
    int cnt = (mv->dwidth-mv->mbase-mv->mwidth)/mv->mwidth;
    int dcnt = MVDisplayedCnt(mv);

    if ( cnt>dcnt ) cnt = dcnt;
    if ( cnt==0 ) cnt = 1;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= cnt;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += cnt;
      break;
      case et_sb_bottom:
        newpos = mv->charcnt-cnt;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>mv->charcnt-cnt )
        newpos = mv->charcnt-cnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=mv->coff ) {
	int old = mv->coff;
	int diff = newpos-mv->coff;
	int charsize = mv->perchar[newpos].dx-mv->perchar[old].dx;
	GRect fieldrect, charrect;

	mv->coff = newpos;
	charrect.x = 0; charrect.width = mv->dwidth;
	charrect.y = mv->topend; charrect.height = mv->displayend-mv->topend;
	fieldrect.x = mv->mbase+mv->mwidth; fieldrect.width = mv->width-mv->mbase;
	fieldrect.y = mv->displayend; fieldrect.height = mv->height-mv->sbh-mv->displayend;
	GScrollBarSetBounds(mv->hsb,0,mv->charcnt,cnt);
	GScrollBarSetPos(mv->hsb,mv->coff);
	MVMoveFieldsBy(mv,newpos*mv->mwidth);
	GDrawScroll(mv->gw,&fieldrect,-diff*mv->mwidth,0);
	mv->xoff = mv->perchar[newpos].dx-mv->perchar[0].dx;
	if ( mv->right_to_left ) {
	    charsize = -charsize;
	}
	GDrawScroll(mv->gw,&charrect,-charsize,0);
    }
}

static void MVVScroll(MetricsView *mv,struct sbevent *sb) {
    int newpos = mv->yoff;
    int32 min, max, page;

    GScrollBarGetBounds(mv->vsb,&min,&max,&page);
    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= page;
      break;
      case et_sb_up:
        newpos -= (page)/15;
      break;
      case et_sb_down:
        newpos += (page)/15;
      break;
      case et_sb_downpage:
        newpos += page;
      break;
      case et_sb_bottom:
        newpos = max-page;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>max-page )
        newpos = max-page;
    if ( newpos<min ) newpos = min;
    if ( newpos!=mv->yoff ) {
	int diff = newpos-mv->yoff;
	GRect charrect;

	mv->yoff = newpos;
	charrect.x = 0; charrect.width = mv->dwidth;
	charrect.y = mv->topend+1; charrect.height = mv->displayend-mv->topend-1;
	GScrollBarSetPos(mv->vsb,mv->yoff);
	GDrawScroll(mv->gw,&charrect,0,diff);
    }
}

void MVSetSCs(MetricsView *mv, SplineChar **scs) {
    /* set the list of characters being displayed to those in scs */
    int len;
    unichar_t *ustr;

    for ( len=0; scs[len]!=NULL; ++len );

    ustr = galloc((len+1)*sizeof(unichar_t));
    for ( len=0; scs[len]!=NULL; ++len )
	if ( scs[len]->unicodeenc>0 && scs[len]->unicodeenc<0x10000 )
	    ustr[len] = scs[len]->unicodeenc;
	else
	    ustr[len] = 0xfffd;
    ustr[len] = 0;
    GGadgetSetTitle(mv->text,ustr);
    free(ustr);

    if ( len>=mv->max ) {
	int oldmax=mv->max;
	mv->max = len+10;
	mv->perchar = grealloc(mv->perchar,mv->max*sizeof(struct metricchar));
	mv->sstr = grealloc(mv->sstr,(mv->max+1)*sizeof(SplineChar *));
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }
    for ( len=0; scs[len]!=NULL; ++len )
	MVSetPos(mv,len,scs[len]);
    if ( len!=0 )
	MVDoSelect(mv,len-1);

    mv->charcnt = len;
    GDrawRequestExpose(mv->gw,NULL,false);
    MVSetSb(mv);
}

static void MVTextChanged(MetricsView *mv) {
    const unichar_t *ret, *pt, *ept, *tpt;
    int i,ei, j,oldx, start=0, end=0;
    static unichar_t nullstr[] = { 0 };
    GRect r;
    int missing;
    int direction_change = false;
    SplineChar **hold = NULL;

    ret = _GGadgetGetTitle(mv->text);
    if (( isrighttoleft(ret[0]) && !mv->right_to_left ) ||
	    ( !isrighttoleft(ret[0]) && mv->right_to_left )) {
	direction_change = true;
	mv->right_to_left = !mv->right_to_left;
    }
    for ( pt=ret, i=0; i<mv->charcnt && *pt!='\0'; ++i, ++pt )
	if ( *pt!=mv->perchar[i].sc->unicodeenc &&
		(*pt!=0xfffd || mv->perchar[i].sc->unicodeenc!=-1 ))
    break;
    if ( i==mv->charcnt && *pt=='\0' )
return;					/* Nothing changed */
    for ( ept=ret+u_strlen(ret)-1, ei=mv->charcnt-1; ; --ei, --ept )
	if ( ei<0 || ept<ret || (*ept!=mv->perchar[ei].sc->unicodeenc &&
		(*ept!=0xfffd || mv->perchar[ei].sc->unicodeenc!=-1 ))) {
	    ++ei; ++ept;
    break;
	} else if ( ei<=i || ept<=pt ) {
	    ++ei; ++ept;
    break;
	}
    /* the change happened between i and ei, and between pt and ept */
    oldx = mv->perchar[i].dx;
    if ( mv->perchar[i].show!=NULL && mv->perchar[i].show->xmin<0 )
	oldx += mv->perchar[i].show->xmin;	/* Beware of negative lbearing */
    if ( i!=0 && oldx > mv->perchar[i-1].dx + mv->perchar[i-1].dwidth ) /* without kern */
	oldx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth;
    if ( ei==i && ept==pt )
	IError("No change when there should have been one in MV_TextChanged");
    if ( u_strlen(ret)>=mv->max ) {
	int oldmax=mv->max;
	mv->max = u_strlen(ret)+10;
	mv->perchar = grealloc(mv->perchar,mv->max*sizeof(struct metricchar));
	mv->sstr = grealloc(mv->sstr,(mv->max+1)*sizeof(SplineChar *));
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }

    missing = 0;
    for ( tpt=pt; tpt<ept; ++tpt )
	if ( SFFindSlot(mv->fv->sf,mv->fv->map,*tpt,NULL)==-1 )
	    ++missing;

    if ( ept-pt-missing > ei-i ) {
	if ( i<mv->charcnt ) {
	    int diff = (ept-pt-missing) - (ei-i);
	    hold = galloc((mv->charcnt+diff+6)*sizeof(SplineChar *));
	    for ( j=mv->charcnt-1; j>=ei; --j )
		hold[j+diff] = mv->perchar[j].sc;
	    start = ei+diff; end = mv->charcnt+diff;
	}
    } else if ( ept-pt-missing != ei-i ) {
	int diff = (ept-pt-missing) - (ei-i);
	for ( j=ei; j<mv->charcnt; ++j )
	    if ( j+diff>=0 )
		MVSetPos(mv,j+diff,mv->perchar[j].sc);
	for ( j+=diff; j<mv->charcnt; ++j ) if ( j>=0 ) {
	    BDFCharFree( mv->perchar[j].show );
	    mv->perchar[j].show = NULL;
	    GGadgetSetTitle(mv->perchar[j].name,nullstr);
	    GGadgetSetTitle(mv->perchar[j].width,nullstr);
	    GGadgetSetTitle(mv->perchar[j].lbearing,nullstr);
	    GGadgetSetTitle(mv->perchar[j].rbearing,nullstr);
	    if ( mv->perchar[j].kern!=NULL )
		GGadgetSetTitle(mv->perchar[j].kern,nullstr);
	}
	mv->charcnt += diff;
    }
    for ( j=i; pt<ept; ++pt ) {
	SplineChar *sc = SCFromUnicode(mv->fv->sf,mv->fv->map,*pt);
	if ( sc!=NULL )
	    MVSetPos(mv,j++,sc);
    }
    if ( hold!=NULL ) {
	/* We had to figure out what sc's there were before we wrote over them*/
	/*  but we couldn't put them where they belonged until everything before*/
	/*  them was set properly */
	for ( j=start; j<end; ++j )
	    MVSetPos(mv,j,hold[j]);
	free(hold);
    }
    r.x = mv->perchar[i].dx;
   /* If i points to eol then mv->perchar[i].dx will be correct but .show will*/
   /*  be NULL */
    if ( mv->perchar[i].show!=NULL && mv->perchar[i].show->xmin<0 )
	r.x += mv->perchar[i].show->xmin;		/* Beware of negative lbearing */
    if ( i!=0 && r.x > mv->perchar[i-1].dx + mv->perchar[i-1].dwidth ) /* without kern */
	r.x = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth;
    if ( r.x>oldx ) r.x = oldx;
    if ( i==0 ) r.x = 0;
    r.width = mv->dwidth;
    if ( MVSetAnchor(mv))
	r.x = 0;
    if ( direction_change || mv->right_to_left )
	r.x = 0;
    r.y = mv->topend; r.height = mv->displayend-r.y;
    r.x -= mv->xoff;
    mv->charcnt = u_strlen(ret)-missing;
    GDrawRequestExpose(mv->gw,&r,false);
    MVSetSb(mv);
}

static int MV_TextChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	MVTextChanged(GGadgetGetUserData(g));
    }
return( true );
}


static int MV_SLIChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MetricsView *mv = GGadgetGetUserData(g);
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	int i,index;
	KernPair *kp;
	KernClass *kc;

	if ( ti[len-1]->selected ) /* Edit script/lang list */
	    ScriptLangList(mv->fv->sf,g,mv->cur_sli);
	else {
	    mv->cur_sli = GGadgetGetFirstListSelectedItem(g);
	    for ( i=0; i<mv->charcnt; ++i ) {
		if ( mv->perchar[i].selected )
	    break;
	    }
	    if ( i!=0 ) {
		for ( kp=kp=mv->vertical ? mv->perchar[i-1].sc->vkerns:mv->perchar[i-1].sc->kerns; kp!=NULL; kp=kp->next ) {
		    if ( kp->sc==mv->perchar[i].sc ) {
			kp->sli = mv->cur_sli;
		break;
		    }
		}
		if ( kp==NULL ) {
		    kc= !mv->vertical
			? SFFindKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,false)
			: SFFindVKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,false);
		    if ( kc!=NULL )
			kc->sli = mv->cur_sli;
		}
	    }
	}
    }
return( true );
}

#define MID_ZoomIn	2002
#define MID_ZoomOut	2003
#define MID_Next	2005
#define MID_Prev	2006
#define MID_Outline	2007
#define MID_ShowGrid	2008
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_AntiAlias	2014
#define MID_FindInFontView	2015
#define MID_Substitutions	2016
#define MID_Ligatures	2020
#define MID_KernPairs	2021
#define MID_AnchorPairs	2022
#define MID_Vertical	2023
#define MID_ReplaceChar	2024
#define MID_InsertCharB	2025
#define MID_InsertCharA	2026
#define MID_CharInfo	2201
#define MID_FindProblems 2216
#define MID_MetaFont	2217
#define MID_Transform	2202
#define MID_Stroke	2203
#define MID_RmOverlap	2204
#define MID_Simplify	2205
#define MID_Correct	2206
#define MID_BuildAccent	2208
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Autotrace	2212
#define MID_Round	2213
#define MID_ShowDependents	2222
#define MID_AddExtrema	2224
#define MID_CleanupGlyph	2225
#define MID_TilePath	2226
#define MID_BuildComposite	2227
#define MID_Intersection	2229
#define MID_FindInter	2230
#define MID_Effects	2231
#define MID_SimplifyMore	2232
#define MID_Center	2600
#define MID_OpenBitmap	2700
#define MID_OpenOutline	2701
#define MID_Display	2706
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_SelAll	2106
#define MID_UnlinkRef	2108
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_CopyRef	2107
#define MID_CopyWidth	2111
#define MID_CopyLBearing	2125
#define MID_CopyRBearing	2126
#define MID_CopyVWidth	2127
#define MID_Join	2128
#define MID_Center	2600
#define MID_Thirds	2604
#define MID_VKernClass	2605
#define MID_VKernFromHKern	2606
#define MID_Recent	2703

#define MID_Warnings	3000

static void MVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GDrawDestroyWindow(gw);
}

static void MVMenuOpenBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    EncMap *map;
    int i;

    if ( mv->fv->sf->bitmaps==NULL )
return;
    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    map = mv->fv->map;
    if ( i!=mv->charcnt )
	BitmapViewCreatePick(map->backmap[mv->perchar[i].sc->orig_pos],mv->fv);
}

static void MVMenuMergeKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MergeKernInfo(mv->fv->sf,mv->fv->map);
}

static void MVMenuOpenOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->charcnt )
	CharViewCreate(mv->perchar[i].sc,mv->fv,-1);
}

static void MVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuSave(mv->fv);
}

static void MVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuSaveAs(mv->fv);
}

static void MVMenuGenerate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuGenerate(mv->fv,false);
}

static void MVMenuGenerateFamily(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuGenerate(mv->fv,true);
}

static void MVMenuPrint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    PrintDlg(NULL,NULL,mv);
}

static void MVMenuDisplay(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    DisplayDlg(mv->fv->sf);
}

static void MVUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_undo) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	if ( mv->perchar[i].sc->layers[ly_fore].undoes!=NULL )
	    SCDoUndo(mv->perchar[i].sc,ly_fore);
    }
}

static void MVRedo(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_redo) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	if ( mv->perchar[i].sc->layers[ly_fore].redoes!=NULL )
	    SCDoRedo(mv->perchar[i].sc,ly_fore);
    }
}

static void MVClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    BDFFont *bdf;
    extern int onlycopydisplayed;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_clear) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	sc = mv->perchar[i].sc;
	if ( sc->dependents!=NULL ) {
	    int yes;
	    char *buts[4];
	    buts[1] = _("_Unlink");
#if defined(FONTFORGE_CONFIG_GDRAW)
	    buts[0] = _("_Yes");
	    buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
	    buts[0] = GTK_STOCK_YES;
	    buts[2] = GTK_STOCK_CANCEL;
#endif
	    buts[3] = NULL;
	    yes = gwwv_ask(_("Bad Reference"),(const char **) buts,1,2,_("You are attempting to clear %.30s which is referred to by\nanother character. Are you sure you want to clear it?"),sc->name);
	    if ( yes==2 )
return;
	    if ( yes==1 )
		UnlinkThisReference(NULL,sc);
	}

	if ( onlycopydisplayed && mv->bdf==NULL ) {
	    SCClearAll(sc);
	} else if ( onlycopydisplayed ) {
	    BCClearAll(mv->bdf->glyphs[sc->orig_pos]);
	} else {
	    SCClearAll(sc);
	    for ( bdf=mv->fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next )
		BCClearAll(bdf->glyphs[sc->orig_pos]);
	}
    }
}

static void MVCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_cut) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	MVCopyChar(mv,mv->perchar[i].sc,true);
	MVClear(gw,mi,e);
    }
}

static void MVCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_copy) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	MVCopyChar(mv,mv->perchar[i].sc,true);
    }
}

static void MVMenuCopyRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    MVCopyChar(mv,mv->perchar[i].sc,false);
}

static void MVMenuCopyWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    SCCopyWidth(mv->perchar[i].sc,
		   mi->mid==MID_CopyWidth?ut_width:
		   mi->mid==MID_CopyVWidth?ut_vwidth:
		   mi->mid==MID_CopyLBearing?ut_lbearing:
					 ut_rbearing);
}

static void MVMenuJoin(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, changed;
    extern float joinsnap;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    SCPreserveState(mv->perchar[i].sc,false);
    mv->perchar[i].sc->layers[ly_fore].splines =
	    SplineSetJoin(mv->perchar[i].sc->layers[ly_fore].splines,true,joinsnap,&changed);
    if ( changed )
	SCCharChangedUpdate(mv->perchar[i].sc);
}

static void MVPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_paste) )
	/*MVTextChanged(mv)*/;		/* Should get an event now */
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	PasteIntoMV(mv,mv->perchar[i].sc,true);
    }
}

static void MVUnlinkRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    RefChar *rf, *next;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    sc = mv->perchar[i].sc;
    SCPreserveState(sc,false);
    for ( rf=sc->layers[ly_fore].refs; rf!=NULL ; rf=next ) {
	next = rf->next;
	SCRefToSplines(sc,rf);
    }
    SCCharChangedUpdate(sc);
}

static void MVSelectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    GGadgetActiveGadgetEditCmd(mv->gw,ec_selectall);
}

static void MVMenuFontInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    DelayEvent(FontMenuFontInfo,mv->fv);
}

static void MVMenuCharInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCCharInfo(mv->perchar[i].sc,mv->fv->map,-1);
}

static void MVMenuShowDependents(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
return;
    if ( mv->perchar[i].sc->dependents==NULL )
return;
    SCRefBy(mv->perchar[i].sc);
}

static void MVMenuFindProblems(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	FindProblems(mv->fv,NULL,mv->perchar[i].sc);
}

static void MVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->charcnt )
	BitmapDlg(mv->fv,mv->perchar[i].sc,mi->mid==MID_AvailBitmaps );
    else if ( mi->mid==MID_AvailBitmaps )
	BitmapDlg(mv->fv,NULL,true );
}

static int getorigin(void *d,BasePoint *base,int index) {
    SplineChar *sc = (SplineChar *) d;
    DBounds bb;

    base->x = base->y = 0;
    switch ( index ) {
      case 0:		/* Character origin */
	/* all done */
      break;
      case 1:		/* Center of selection */
	SplineCharFindBounds(sc,&bb);
	base->x = (bb.minx+bb.maxx)/2;
	base->y = (bb.miny+bb.maxy)/2;
      break;
      default:
return( false );
    }
return( true );
}

static void MVTransFunc(void *_sc,real transform[6],int otype, BVTFunc *bvts,
	enum fvtrans_flags flags ) {
    SplineChar *sc = _sc;

    FVTrans(sc->parent->fv,sc,transform, NULL,flags);
}

static void MVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	TransformDlgCreate( mv->perchar[i].sc,MVTransFunc,getorigin,true,cvt_none );
}

static void MVMenuStroke(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCStroke(mv->perchar[i].sc);
}

#ifdef FONTFORGE_CONFIG_TILEPATH
static void MVMenuTilePath(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCTile(mv->perchar[i].sc);
}
#endif

static void _MVMenuOverlap(MetricsView *mv,enum overlap_type ot) {
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	if ( !SCRoundToCluster(sc,-2,false,.03,.12))
	    SCPreserveState(sc,false);
	MinimumDistancesFree(sc->md);
	sc->md = NULL;
	sc->layers[ly_fore].splines = SplineSetRemoveOverlap(sc,sc->layers[ly_fore].splines,ot);
	SCCharChangedUpdate(sc);
    }
}

static void MVMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuOverlap(mv,mi->mid==MID_RmOverlap ? over_remove :
		      mi->mid==MID_Intersection ? over_intersect :
			   over_findinter);
}

static void MVMenuInline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    OutlineDlg(NULL,NULL,mv,true);
}

static void MVMenuOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    OutlineDlg(NULL,NULL,mv,false);
}

static void MVMenuShadow(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    ShadowDlg(NULL,NULL,mv,false);
}

static void MVMenuWireframe(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    ShadowDlg(NULL,NULL,mv,true);
}

static void MVSimplify( MetricsView *mv,int type ) {
    int i;
    static struct simplifyinfo smpls[] = {
	    { sf_normal },
	    { sf_normal,.75,.05,0,-1 },
	    { sf_normal,.75,.05,0,-1 }};
    struct simplifyinfo *smpl = &smpls[type+1];

    if ( smpl->linelenmax==-1 ) {
	smpl->err = (mv->fv->sf->ascent+mv->fv->sf->descent)/1000.;
	smpl->linelenmax = (mv->fv->sf->ascent+mv->fv->sf->descent)/100.;
    }

    if ( type==1 ) {
	if ( !SimplifyDlg(mv->fv->sf,smpl))
return;
	if ( smpl->set_as_default )
	    smpls[1] = *smpl;
    }

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	sc->layers[ly_fore].splines = SplineCharSimplify(sc,sc->layers[ly_fore].splines,smpl);
	SCCharChangedUpdate(sc);
    }
}

static void MVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv,false);
}

static void MVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv,true);
}

static void MVMenuCleanup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv,-1);
}

static void MVMenuAddExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	SplineCharAddExtrema(sc,sc->layers[ly_fore].splines,ae_only_good,sc->parent);
	SCCharChangedUpdate(sc);
    }
}

static void MVMenuRound2Int(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SCPreserveState(mv->perchar[i].sc,false);
	SCRound2Int( mv->perchar[i].sc,1.0);
    }
}

static void MVMenuMetaFont(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	MetaFont(NULL, NULL, mv->perchar[i].sc);
}

static void MVMenuAutotrace(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCAutoTrace(mv->perchar[i].sc,mv->gw,e!=NULL && (e->u.mouse.state&ksm_shift));
}

static void MVMenuCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	int changed = false, refchanged=false;
	RefChar *ref;
	int asked=-1;

	for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    if ( ref->transform[0]*ref->transform[3]<0 ||
		    (ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		if ( asked==-1 ) {
		    char *buts[4];
		    buts[0] = _("_Unlink");
#if defined(FONTFORGE_CONFIG_GDRAW)
		    buts[1] = _("_No");
		    buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
		    buts[1] = GTK_RESPONSE_NO;
		    buts[2] = GTK_RESPONSE_CANCEL;
#endif
		    buts[3] = NULL;
		    asked = gwwv_ask(_("Flipped Reference"),(const char **) buts,0,2,_("%.50s contains a flipped reference. This cannot be corrected as is. Would you like me to unlink it and then correct it?"), sc->name );
		    if ( asked==2 )
return;
		    else if ( asked==1 )
	break;
		}
		if ( asked==0 ) {
		    if ( !refchanged ) {
			refchanged = true;
			SCPreserveState(sc,false);
		    }
		    SCRefToSplines(sc,ref);
		}
	    }
	}

	if ( !refchanged )
	    SCPreserveState(sc,false);
	sc->layers[ly_fore].splines = SplineSetsCorrect(sc->layers[ly_fore].splines,&changed);
	if ( changed || refchanged )
	    SCCharChangedUpdate(sc);
    }
}

static void _MVMenuBuildAccent(MetricsView *mv,int onlyaccents) {
    int i;
    extern int onlycopydisplayed;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	if ( SFIsSomethingBuildable(mv->fv->sf,sc,onlyaccents) )
	    SCBuildComposit(mv->fv->sf,sc,!onlycopydisplayed,mv->fv);
    }
}

static void MVMenuBuildAccent(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuBuildAccent(mv,false);
}

static void MVMenuBuildComposite(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuBuildAccent(mv,true);
}

static void MVResetText(MetricsView *mv) {
    unichar_t *new, *pt;
    int i,si;

    new = galloc((mv->charcnt+1)*sizeof(unichar_t));
    si=-1;
    for ( pt=new, i=0; i<mv->charcnt; ++i ) {
	if ( mv->perchar[i].sc->unicodeenc==-1 || mv->perchar[i].sc->unicodeenc>=0x10000 )
	    *pt++ = 0xfffd;
	else
	    *pt++ = mv->perchar[i].sc->unicodeenc;
	if ( mv->perchar[i].selected ) si=i;
    }
    *pt = '\0';
    GGadgetSetTitle(mv->text,new);
    free(new );
}

static void MVMenuLigatures(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowLigatures(mv->fv->sf,NULL);
}

static void MVMenuKernPairs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowKernPairs(mv->fv->sf,NULL,NULL);
}

static void MVMenuAnchorPairs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowKernPairs(mv->fv->sf,NULL,mi->ti.userdata);
}

static void MVMenuScale(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    if ( mi->mid==MID_ZoomIn ) {
	if ( --mv->scale_index<0 ) mv->scale_index = 0;
    } else {
	if ( ++mv->scale_index >= sizeof(mv_scales)/sizeof(mv_scales[0]) )
	    mv->scale_index = sizeof(mv_scales)/sizeof(mv_scales[0])-1;
    }

    mv->pixelsize = mv_scales[mv->scale_index]*(mv->displayend - mv->topend - 4);
    MVReRasterize(mv);
    MVReKern(mv);
    MVSetVSb(mv);
}

static void MVMenuInsertChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->fv->sf;
    int i, j, pos = GotoChar(sf,mv->fv->map);

    if ( pos==-1 || pos>=mv->fv->map->enccount )
return;

    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( mi->mid==MID_InsertCharA ) {
	if ( i==mv->charcnt ) i = mv->charcnt-1;
	++i;
    } else {
	if ( i==mv->charcnt ) i = 0;
    }

    if ( mv->charcnt+1>=mv->max ) {
	int oldmax=mv->max;
	mv->max = mv->charcnt+10;
	mv->perchar = grealloc(mv->perchar,mv->max*sizeof(struct metricchar));
	mv->sstr = grealloc(mv->sstr,(mv->max+1)*sizeof(SplineChar *));
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }
    for ( j=mv->charcnt; j>i; --j )
	MVSetPos(mv,j,mv->perchar[j-1].sc);
    MVSetPos(mv,i,SFMakeChar(sf,mv->fv->map,pos));
    MVDoSelect(mv,i);
    for ( ; i<mv->charcnt; ++i )
	if ( i!=0 )
	    mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth + mv->perchar[i-1].kernafter + mv->perchar[i-1].hoff;
    MVSetAnchor(mv);
    GDrawRequestExpose(mv->gw,NULL,false);
    MVResetText(mv);
}

static void MVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->fv->sf;
    EncMap *map = mv->fv->map;
    int i, pos, gid;

    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->charcnt ) {
	pos = -1;
	if ( mi->mid == MID_Next ) {
	    pos = map->backmap[mv->perchar[i].sc->orig_pos]+1;
	} else if ( mi->mid==MID_Prev ) {
	    pos = map->backmap[mv->perchar[i].sc->orig_pos]-1;
	} else if ( mi->mid==MID_NextDef ) {
	    for ( pos = map->backmap[mv->perchar[i].sc->orig_pos]+1;
		    pos<map->enccount && ((gid=map->map[pos])==-1 || sf->glyphs[gid]==NULL); ++pos );
	    if ( pos>=map->enccount )
return;
	} else if ( mi->mid==MID_PrevDef ) {
	    for ( pos = map->backmap[mv->perchar[i].sc->orig_pos]-1;
		    pos<map->enccount && ((gid=map->map[pos])==-1 || sf->glyphs[gid]==NULL); --pos );
	    if ( pos<0 )
return;
	} else if ( mi->mid==MID_ReplaceChar ) {
	    pos = GotoChar(sf,mv->fv->map);
	    if ( pos<0 )
return;
	}
	if ( pos>=0 && pos<map->enccount ) {
	    MVSetPos(mv,i,SFMakeChar(sf,mv->fv->map,pos));
	    /* May need to adjust start of current char if kerning changed */
	    for ( ; i<mv->charcnt; ++i )
		if ( i!=0 )
		    mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth + mv->perchar[i-1].kernafter + mv->perchar[i-1].hoff;
	    MVSetAnchor(mv);
	    GDrawRequestExpose(mv->gw,NULL,false);
	    MVResetText(mv);
	}
    }
}

static void MVMenuFindInFontView(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->charcnt; ++i ) {
	if ( mv->perchar[i].selected ) {
	    FVChangeChar(mv->fv,mv->fv->map->backmap[mv->perchar[i].sc->orig_pos]);
	    GDrawSetVisible(mv->fv->gw,true);
	    GDrawRaise(mv->fv->gw);
    break;
	}
    }
}

static void MVMenuShowGrid(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    mv->showgrid = !mv->showgrid;
    GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVMenuAA(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    mv_antialias = mv->antialias = !mv->antialias;
    MVReRasterize(mv);
    GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVMenuVertical(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    if ( !mv->fv->sf->hasvmetrics ) {
	if ( mv->vertical )
	    MVToggleVertical(mv);
    } else
	MVToggleVertical(mv);
    GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVMenuShowBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    BDFFont *bdf = mi->ti.userdata;

    if ( mv->bdf!=bdf ) {
	MVChangeDisplayFont(mv,bdf);
	GDrawRequestExpose(mv->gw,NULL,false);
    }
}

static void MVMenuCenter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    DBounds bb;
    real transform[6];
    SplineChar *sc;

    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->charcnt ) {
	sc = mv->perchar[i].sc;
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0.0;
	SplineCharFindBounds(sc,&bb);
	if ( mi->mid==MID_Center )
	    transform[4] = (sc->width-(bb.maxx-bb.minx))/2 - bb.minx;
	else
	    transform[4] = (sc->width-(bb.maxx-bb.minx))/3 - bb.minx;
	if ( transform[4]!=0 )
	    FVTrans(mv->fv,sc,transform,NULL,fvt_dontmovewidth);
    }
}

static void MVMenuKernByClasses(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    ShowKernClasses(mv->fv->sf,mv,false);
}

static void MVMenuVKernByClasses(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    ShowKernClasses(mv->fv->sf,mv,true);
}

static void MVMenuVKernFromHKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    FVVKernFromHKern(mv->fv);
}

static void MVMenuKPCloseup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineChar *sc1=NULL, *sc2=NULL;
    int i;

    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected ) {
	    sc1 = mv->perchar[i].sc;
	    if ( i+1<mv->charcnt )
		sc2 = mv->perchar[i+1].sc;
    break;
	}
    KernPairD(mv->fv->sf,sc1,sc2,mv->vertical);
}

static void PosSubsMenuFree(GMenuItem *mi) {
    int i;

    if ( mi==NULL )
return;
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.line; ++i ) {
	if ( !mi[i].ti.text_is_1byte )
	    free( mi[i].ti.text );
	if ( mi[i].sub!=NULL )
	    PosSubsMenuFree(mi[i].sub);
    }
    free(mi);
}

static void MVRevertSubs(GWindow gw, GMenuItem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->charcnt; ++i ) if ( mv->perchar[i].selected ) {
	SplineChar *basesc = mv->perchar[i].basesc;
	if ( basesc==NULL )
return;
	MVSetPos(mv,i,basesc);
	/* Should I update the text field??? */
	MVSetAnchor(mv);
	GDrawRequestExpose(mv->gw,NULL,false);
    break;
    }
}

static void MVSubsInvoked(GWindow gw, GMenuItem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->charcnt; ++i ) if ( mv->perchar[i].selected ) {
	char *name = mi->ti.userdata, *pt;
	SplineChar *sc;
	SplineChar *basesc = mv->perchar[i].basesc;
	if ( basesc==NULL ) basesc = mv->perchar[i].sc;
	pt = strchr(name,' ');
	if ( pt!=NULL ) *pt = '\0';
	sc = SFGetChar(mv->fv->sf,-1,name);
	if ( pt!=NULL ) *pt = ' ';

	if ( sc==NULL )
	    gwwv_post_error(_("Could not find the glyph"), _("Could not find substitution glyph: %.40s"), name);
	else {
	    MVSetPos(mv,i,sc);
	    mv->perchar[i].basesc = basesc;
	    /* Should I update the text field??? */
	    MVSetAnchor(mv);
	    GDrawRequestExpose(mv->gw,NULL,false);
	}
    break;
    }
}

static void GMenuFillWithTag(GMenuItem *mi,GTextInfo *tags,uint32 tag) {
    int j;
    unichar_t ubuf[8];

    for ( j=0; tags[j].text!=NULL; ++j )
	if ( (uint32) (intpt) (tags[j].userdata)==tag )
    break;
    if ( tags[j].text==NULL ) {
	ubuf[0] = tag>>24;
	ubuf[1] = (tag>>16) & 0xff;
	ubuf[2] = (tag>>8 ) & 0xff;
	ubuf[3] = (tag    ) & 0xff;
	ubuf[4] = 0;
	mi->ti.text = u_copy(ubuf);
    } else {
	mi->ti.text = tags[j].text;
	mi->ti.text_is_1byte = true;
    }
    mi->ti.fg = COLOR_DEFAULT;
    mi->ti.bg = COLOR_DEFAULT;
}

static void GMenuMakeSubSub(GMenuItem *mi,char *start,char *pt) {
    mi->ti.text = uc_copyn(start,pt-start);
    mi->ti.fg = COLOR_DEFAULT;
    mi->ti.bg = COLOR_DEFAULT;
    mi->ti.userdata = start;
    mi->invoke = MVSubsInvoked;
}

static GMenuItem *SubsMenuBuild(SplineChar *sc,SplineChar *basesc) {
    PST *pst;
    GMenuItem *mi;
    int cnt, i, j;
    char *pt, *start;
    extern GTextInfo simplesubs_tags[];
    extern GTextInfo alternatesubs_tags[];

    for ( cnt = 0, pst=sc->possub; pst!=NULL ; pst=pst->next )
	if ( pst->type==pst_substitution || pst->type==pst_alternate )
	    ++cnt;
    if ( basesc!=NULL ) {
	if ( cnt>0 ) cnt+=2;
	else cnt = 1;
    }
    if ( cnt==0 )
return( NULL );
    mi = gcalloc(cnt+2,sizeof(GMenuItem));
    CharInfoInit();
    for ( i = 0, pst=sc->possub; pst!=NULL ; pst=pst->next ) {
	if ( pst->type==pst_substitution ) {
	    GMenuFillWithTag(&mi[i],simplesubs_tags,pst->tag);
	    mi[i].ti.userdata = pst->u.subs.variant;
	    mi[i++].invoke = MVSubsInvoked;
	} else if ( pst->type==pst_alternate ) {
	    GMenuFillWithTag(&mi[i],alternatesubs_tags,pst->tag);
	    if ( strchr(pst->u.alt.components,' ')==NULL ) {
		mi[i].ti.userdata = pst->u.alt.components;
		mi[i++].invoke = MVSubsInvoked;
	    } else {
		for ( pt = pst->u.alt.components, j=0; *pt; ++pt )
		    if ( *pt ==' ' ) ++j;
		mi[i].sub = gcalloc(j+2,sizeof(GMenuItem));
		for ( pt = start = pst->u.alt.components, j=0; *pt; ++pt ) if ( *pt ==' ' ) {
		    GMenuMakeSubSub(&mi[i].sub[j],start,pt);
		    start = pt+1;
		    ++j;
		}
		GMenuMakeSubSub(&mi[i].sub[j],start,pt);
		++i;
	    }
	}
    }
    if ( basesc!=NULL ) {
	if ( i!=0 )
	    mi[i++].ti.line = true;
	mi[i].ti.text = (unichar_t *) copy(_("Revert Substitution"));
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].ti.userdata = basesc;
	mi[i].invoke = MVRevertSubs;
    }
return( mi );
}

static void MVPopupInvoked(GWindow gw, GMenuItem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->charcnt; ++i ) if ( mv->perchar[i].selected ) {
	mv->perchar[i].active_pos = mi->ti.userdata;
	MVSetAnchor(mv);
	MVRedrawI(mv,i,0,0);
    break;
    }
}

static void MVPopupMenu(MetricsView *mv,GEvent *event,int sel) {
    PST *pst;
    int cnt, i;
    GMenuItem *mi;
    extern GTextInfo simplepos_tags[];

    for ( cnt=0, pst=mv->perchar[sel].sc->possub; pst!=NULL; pst=pst->next )
	if ( pst->type == pst_position )
	    ++cnt;
    mi = gcalloc(cnt+4,sizeof(GMenuItem));

    mi[0].ti.text = (unichar_t *) _("Plain");
    mi[0].ti.text_is_1byte = true;
    mi[0].ti.fg = COLOR_DEFAULT;
    mi[0].ti.bg = COLOR_DEFAULT;
    mi[0].invoke = MVPopupInvoked;

    if ( cnt!=0 ) {
	mi[1].ti.fg = COLOR_DEFAULT;
	mi[1].ti.bg = COLOR_DEFAULT;
	mi[1].ti.line = true;
    }

    i = 2;
    for ( pst=mv->perchar[sel].sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_position ) {
	    GMenuFillWithTag(&mi[i],simplepos_tags,pst->tag);
	    mi[i].ti.userdata = pst;
	    mi[i++].invoke = MVPopupInvoked;
	}
    }
    GMenuCreatePopupMenu(mv->gw,event, mi);
    PosSubsMenuFree(mi);
}

static GMenuItem wnmenu[] = {
    { { (unichar_t *) N_("New O_utline Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, 'H', ksm_control, NULL, NULL, MVMenuOpenOutline, MID_OpenOutline },
    { { (unichar_t *) N_("New _Bitmap Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, 'J', ksm_control, NULL, NULL, MVMenuOpenBitmap, MID_OpenBitmap },
    { { (unichar_t *) N_("New _Metrics Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 'K', ksm_control, NULL, NULL, /* No function, never avail */NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Warnings"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, _MenuWarnings, MID_Warnings },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { NULL }
};

static void MVWindowMenuBuild(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    struct gmenuitem *wmi;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->perchar[i].sc;

    for ( wmi = wnmenu; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case MID_OpenOutline:
	    wmi->ti.disabled = sc==NULL;
	  break;
	  case MID_OpenBitmap:
	    mi->ti.disabled = mv->fv->sf->bitmaps==NULL || sc==NULL;
	  break;
	  case MID_Warnings:
	    wmi->ti.disabled = ErrorWindowExists();
	  break;
	}
    }
    WindowMenuBuild(gw,mi,e,wnmenu);
}

static GMenuItem dummyitem[] = { { (unichar_t *) N_("_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) N_("_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, 'N', ksm_control, NULL, NULL, MenuNew },
    { { (unichar_t *) N_("_Open"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) N_("Recen_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) N_("_Close"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, MVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, MVMenuSave },
    { { (unichar_t *) N_("S_ave as..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, MVMenuSaveAs },
    { { (unichar_t *) N_("_Generate Fonts..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, 'G', ksm_control|ksm_shift, NULL, NULL, MVMenuGenerate },
    { { (unichar_t *) N_("Generate Mac _Family..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 'G', ksm_control|ksm_meta, NULL, NULL, MVMenuGenerateFamily },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Merge Kern Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 'K', ksm_control|ksm_shift, NULL, NULL, MVMenuMergeKern },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Print..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 'P', ksm_control, NULL, NULL, MVMenuPrint },
    { { (unichar_t *) N_("_Display..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, 'P', ksm_control|ksm_meta, NULL, NULL, MVMenuDisplay, MID_Display },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Pr_eferences..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Quit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) N_("_Undo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL, MVUndo, MID_Undo },
    { { (unichar_t *) N_("_Redo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL, MVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Cu_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, MVCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, MVCopy, MID_Copy },
    { { (unichar_t *) N_("C_opy Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, 'G', ksm_control, NULL, NULL, MVMenuCopyRef, MID_CopyRef },
    { { (unichar_t *) N_("Copy _Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, 'W', ksm_control, NULL, NULL, MVMenuCopyWidth, MID_CopyWidth },
    { { (unichar_t *) N_("Copy _VWidth"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, '\0', ksm_control, NULL, NULL, MVMenuCopyWidth, MID_CopyVWidth },
    { { (unichar_t *) N_("Co_py LBearing"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'p' }, '\0', ksm_control, NULL, NULL, MVMenuCopyWidth, MID_CopyLBearing },
    { { (unichar_t *) N_("Copy RBearin_g"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'g' }, '\0', ksm_control, NULL, NULL, MVMenuCopyWidth, MID_CopyRBearing },
    { { (unichar_t *) N_("_Paste"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, MVPaste, MID_Paste },
    { { (unichar_t *) N_("C_lear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, 0, 0, NULL, NULL, MVClear, MID_Clear },
    { { (unichar_t *) N_("_Join"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'J' }, 'J', ksm_control|ksm_shift, NULL, NULL, MVMenuJoin, MID_Join },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Select _All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, MVSelectAll, MID_SelAll },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("U_nlink Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, 'U', ksm_control, NULL, NULL, MVUnlinkRef, MID_UnlinkRef },
    { NULL }
};

static GMenuItem smlist[] = {
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 'M', ksm_control|ksm_shift, NULL, NULL, MVMenuSimplify, MID_Simplify },
    { { (unichar_t *) N_("Simplify More..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 'M', ksm_control|ksm_shift|ksm_meta, NULL, NULL, MVMenuSimplifyMore, MID_SimplifyMore },
    { { (unichar_t *) N_("Clea_nup Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuCleanup, MID_CleanupGlyph },
    { NULL }
};

static GMenuItem rmlist[] = {
    { { (unichar_t *) N_("_Remove Overlap"), &GIcon_rmoverlap, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, 'O', ksm_control|ksm_shift, NULL, NULL, MVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) N_("_Intersect"), &GIcon_intersection, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuOverlap, MID_Intersection },
    { { (unichar_t *) N_("_Find Intersections"), &GIcon_findinter, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuOverlap, MID_FindInter },
    { NULL }
};

static GMenuItem eflist[] = {
    { { (unichar_t *) N_("_Inline"), &GIcon_inline, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuInline },
    { { (unichar_t *) N_("_Outline"), &GIcon_outline, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuOutline },
    { { (unichar_t *) N_("_Shadow"), &GIcon_shadow, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'S' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuShadow },
    { { (unichar_t *) N_("_Wireframe"), &GIcon_wireframe, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'W' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuWireframe },
    { NULL }
};

static GMenuItem balist[] = {
    { { (unichar_t *) N_("_Build Accented Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, 'A', ksm_control|ksm_shift, NULL, NULL, MVMenuBuildAccent, MID_BuildAccent },
    { { (unichar_t *) N_("Build _Composite Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuBuildComposite, MID_BuildComposite },
    { NULL }
};

static void balistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->perchar[i].sc;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_BuildAccent:
	    mi->ti.disabled = sc==NULL || !SFIsSomethingBuildable(sc->parent,sc,true);
	  break;
	  case MID_BuildComposite:
	    mi->ti.disabled = sc==NULL || !SFIsSomethingBuildable(sc->parent,sc,false);
	  break;
        }
    }
}

static GMenuItem ellist[] = {
    { { (unichar_t *) N_("_Font Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 'F', ksm_control|ksm_shift, NULL, NULL, MVMenuFontInfo },
    { { (unichar_t *) N_("Glyph _Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, 'I', ksm_control, NULL, NULL, MVMenuCharInfo, MID_CharInfo },
    { { (unichar_t *) N_("S_how Dependent"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, 'I', ksm_control|ksm_meta, NULL, NULL, MVMenuShowDependents, MID_ShowDependents },
    { { (unichar_t *) N_("Find Pr_oblems..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, 'E', ksm_control, NULL, NULL, MVMenuFindProblems, MID_FindProblems },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Bitm_aps Available..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, 'B', ksm_control|ksm_shift, NULL, NULL, MVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) N_("Regenerate _Bitmaps..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, 'B', ksm_control, NULL, NULL, MVMenuBitmaps, MID_RegenBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\\', ksm_control, NULL, NULL, MVMenuTransform, MID_Transform },
    { { (unichar_t *) N_("_Expand Stroke..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, 'E', ksm_control|ksm_shift, NULL, NULL, MVMenuStroke, MID_Stroke },
#ifdef FONTFORGE_CONFIG_TILEPATH
    { { (unichar_t *) N_("Tile _Path..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuTilePath, MID_TilePath },
#endif
    { { (unichar_t *) N_("_Remove Overlap"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, '\0', ksm_control|ksm_shift, rmlist, NULL, NULL, MID_RmOverlap },
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, '\0', ksm_control|ksm_shift, smlist, NULL, NULL, MID_Simplify },
    { { (unichar_t *) N_("Add E_xtrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, 'X', ksm_control|ksm_shift, NULL, NULL, MVMenuAddExtrema, MID_AddExtrema },
    { { (unichar_t *) N_("To _Int"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, '_', ksm_control|ksm_shift, NULL, NULL, MVMenuRound2Int, MID_Round },
    { { (unichar_t *) N_("Effects"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control|ksm_shift, eflist, NULL, NULL, MID_Effects },
    { { (unichar_t *) N_("_Meta Font..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '!', ksm_control|ksm_shift, NULL, NULL, MVMenuMetaFont, MID_MetaFont },
    { { (unichar_t *) N_("Autot_race"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, 'T', ksm_control|ksm_shift, NULL, NULL, MVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Correct Direction"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, 'D', ksm_control|ksm_shift, NULL, NULL, MVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("B_uild"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, 'A', ksm_control|ksm_shift, balist, balistcheck, NULL, MID_BuildAccent },
    { NULL }
};

static GMenuItem dummyall[] = {
    { { (unichar_t *) N_("All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, '\0', ksm_shift|ksm_control, NULL, NULL, NULL },
    NULL
};

/* Builds up a menu containing all the anchor classes */
static void aplistbuild(GWindow base,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(base);
    extern void GMenuItemArrayFree(GMenuItem *mi);

    if ( mi->sub!=dummyall ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }

    _aplistbuild(mi,mv->fv->sf,MVMenuAnchorPairs);
}

static GMenuItem cblist[] = {
    { { (unichar_t *) N_("_Kern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, '\0', ksm_shift|ksm_control, NULL, NULL, MVMenuKernPairs, MID_KernPairs },
    { { (unichar_t *) N_("_Anchored Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, '\0', ksm_shift|ksm_control, dummyall, aplistbuild, MVMenuAnchorPairs, MID_AnchorPairs },
    { { (unichar_t *) N_("_Ligatures"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'L' }, '\0', ksm_shift|ksm_control, NULL, NULL, MVMenuLigatures, MID_Ligatures },
    NULL
};

static void cblistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->fv->sf;
    int i, anyligs=0, anykerns=0;
    PST *pst;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type==pst_ligature ) {
		anyligs = true;
		if ( anykerns )
    break;
	    }
	}
	if ( (mv->vertical ? sf->glyphs[i]->vkerns : sf->glyphs[i]->kerns)!=NULL ) {
	    anykerns = true;
	    if ( anyligs )
    break;
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Ligatures:
	    mi->ti.disabled = !anyligs;
	  break;
	  case MID_KernPairs:
	    mi->ti.disabled = !anykerns;
	  break;
	  case MID_AnchorPairs:
	    mi->ti.disabled = sf->anchor==NULL;
	  break;
	}
    }
}

static GMenuItem vwlist[] = {
    { { (unichar_t *) N_("Z_oom out"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '-', ksm_control|ksm_meta, NULL, NULL, MVMenuScale, MID_ZoomOut },
    { { (unichar_t *) N_("Zoom _in"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'i' }, '+', ksm_shift|ksm_control|ksm_meta, NULL, NULL, MVMenuScale, MID_ZoomIn },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Insert Glyph _After..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, MVMenuInsertChar, MID_InsertCharA },
    { { (unichar_t *) N_("Insert Glyph _Before..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, '\0', ksm_control, NULL, NULL, MVMenuInsertChar, MID_InsertCharB },
    { { (unichar_t *) N_("_Replace Glyph..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'G', ksm_control, NULL, NULL, MVMenuChangeChar, MID_ReplaceChar },
    { { (unichar_t *) N_("_Next Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, ']', ksm_control, NULL, NULL, MVMenuChangeChar, MID_Next },
    { { (unichar_t *) N_("_Prev Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '[', ksm_control, NULL, NULL, MVMenuChangeChar, MID_Prev },
    { { (unichar_t *) N_("Next _Defined Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, ']', ksm_control|ksm_meta, NULL, NULL, MVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) N_("Prev Defined Gl_yph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, '[', ksm_control|ksm_meta, NULL, NULL, MVMenuChangeChar, MID_PrevDef },
    { { (unichar_t *) N_("Find In Font _View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, '<', ksm_shift|ksm_control, NULL, NULL, MVMenuFindInFontView, MID_FindInFontView },
    { { (unichar_t *) N_("_Substitutions"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, '\0', ksm_shift|ksm_control, NULL, NULL, NULL, MID_Substitutions },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Com_binations"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'b' }, '\0', ksm_shift|ksm_control, cblist, cblistcheck },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Hide _Grid"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, '\0', ksm_control, NULL, NULL, MVMenuShowGrid, MID_ShowGrid },
    { { (unichar_t *) N_("_Anti Alias"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'A' }, '5', ksm_control, NULL, NULL, MVMenuAA, MID_AntiAlias },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Vertical"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, MVMenuVertical, MID_Vertical },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'O' }, '\0', ksm_control, NULL, NULL, MVMenuShowBitmap, MID_Outline },
    { NULL },			/* Some extra room to show bitmaps */
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }
};

# ifdef FONTFORGE_CONFIG_GDRAW
static void MVMenuContextualHelp(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void MetricsViewMenu_ContextualHelp(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    help("metricsview.html");
}

static GMenuItem mtlist[] = {
    { { (unichar_t *) N_("_Center in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, MVMenuCenter, MID_Center },
    { { (unichar_t *) N_("_Thirds in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, MVMenuCenter, MID_Thirds },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Ker_n By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, MVMenuKernByClasses },
    { { (unichar_t *) N_("VKern By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, MVMenuVKernByClasses, MID_VKernClass },
    { { (unichar_t *) N_("VKern From HKern"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, MVMenuVKernFromHKern, MID_VKernFromHKern },
    { { (unichar_t *) N_("Kern Pair Closeup..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, MVMenuKPCloseup },
    { NULL }
};

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	  case MID_Display:
	    mi->ti.disabled = (mv->fv->sf->onlybitmaps && mv->fv->sf->bitmaps==NULL) ||
		    mv->fv->sf->multilayer;
	  break;
	}
    }
}

static void edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
	i = -1;
    else
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Join:
	  case MID_Copy: case MID_CopyRef: case MID_CopyWidth:
	  case MID_CopyLBearing: case MID_CopyRBearing:
	  case MID_Cut: case MID_Clear:
	    mi->ti.disabled = i==-1;
	  break;
	  case MID_CopyVWidth:
	    mi->ti.disabled = i==-1 || !mv->fv->sf->hasvmetrics;
	  break;
	  case MID_Undo:
	    mi->ti.disabled = i==-1 || mv->perchar[i].sc->layers[ly_fore].undoes==NULL;
	  break;
	  case MID_Redo:
	    mi->ti.disabled = i==-1 || mv->perchar[i].sc->layers[ly_fore].redoes==NULL;
	  break;
	  case MID_UnlinkRef:
	    mi->ti.disabled = i==-1 || mv->perchar[i].sc->layers[ly_fore].refs==NULL;
	  break;
	  case MID_Paste:
	    mi->ti.disabled = i==-1 ||
		(!CopyContainsSomething() &&
#ifndef _NO_LIBPNG
		    !GDrawSelectionHasType(mv->gw,sn_clipboard,"image/png") &&
#endif
		    !GDrawSelectionHasType(mv->gw,sn_clipboard,"image/bmp") &&
		    !GDrawSelectionHasType(mv->gw,sn_clipboard,"image/eps"));
	  break;
	}
    }
}

static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, anybuildable;
    SplineChar *sc;
    int order2 = mv->fv->sf->order2;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->perchar[i].sc;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_RegenBitmaps:
	    mi->ti.disabled = mv->fv->sf->bitmaps==NULL;
	  break;
	  case MID_CharInfo:
	    mi->ti.disabled = sc==NULL /*|| mv->fv->cidmaster!=NULL*/;
	  break;
	  case MID_ShowDependents:
	    mi->ti.disabled = sc==NULL || sc->dependents == NULL;
	  break;
	  case MID_MetaFont:
	    mi->ti.disabled = sc==NULL || order2;
	  break;
	  case MID_FindProblems:
	  case MID_Transform:
	    mi->ti.disabled = sc==NULL;
	  break;
	  case MID_Effects:
	    mi->ti.disabled = sc==NULL || mv->fv->sf->onlybitmaps || order2;
	  break;
	  case MID_RmOverlap: case MID_Stroke:
	    mi->ti.disabled = sc==NULL || mv->fv->sf->onlybitmaps;
	  break;
	  case MID_AddExtrema:
	  case MID_Round: case MID_Correct:
	    mi->ti.disabled = sc==NULL || mv->fv->sf->onlybitmaps;
	  break;
#ifdef FONTFORGE_CONFIG_TILEPATH
	  case MID_TilePath:
	    mi->ti.disabled = sc==NULL || mv->fv->sf->onlybitmaps || ClipBoardToSplineSet()==NULL || order2;
	  break;
#endif
	  case MID_Simplify:
	    mi->ti.disabled = sc==NULL || mv->fv->sf->onlybitmaps;
	  break;
	  case MID_BuildAccent:
	    anybuildable = false;
	    if ( sc!=NULL && SFIsSomethingBuildable(mv->fv->sf,sc,false) )
		anybuildable = true;
	    mi->ti.disabled = !anybuildable;
	  break;
	  case MID_Autotrace:
	    mi->ti.disabled = !(FindAutoTraceName()!=NULL && sc!=NULL &&
		    sc->layers[ly_back].images!=NULL );
	  break;
	}
    }
}

static void vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, j, base, aselection;
    BDFFont *bdf;
    char buffer[60];
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt);

    aselection = false;
    for ( j=0; j<mv->charcnt; ++j )
	if ( mv->perchar[j].selected ) {
	    aselection = true;
    break;
	}

    for ( i=0; vwlist[i].mid!=MID_Outline; ++i )
	switch ( vwlist[i].mid ) {
	  case MID_ZoomIn:
	    vwlist[i].ti.disabled = mv->bdf!=NULL || mv->scale_index==0;
	  break;
	  case MID_ZoomOut:
	    vwlist[i].ti.disabled = mv->bdf!=NULL || mv->scale_index>=sizeof(mv_scales)/sizeof(mv_scales[0])-1;
	  break;
	  case MID_ShowGrid:
	    vwlist[i].ti.text = (unichar_t *) (mv->showgrid?_("Hide Grid"):_("Show _Grid"));
	  break;
	  case MID_AntiAlias:
	    vwlist[i].ti.checked = mv->antialias;
	    vwlist[i].ti.disabled = mv->bdf!=NULL;
	  break;
	  case MID_ReplaceChar:
	  case MID_FindInFontView:
	  case MID_Next:
	  case MID_Prev:
	  case MID_NextDef:
	  case MID_PrevDef:
	    vwlist[i].ti.disabled = !aselection;
	  break;
	  case MID_Substitutions:
	    PosSubsMenuFree(vwlist[i].sub);
	    vwlist[i].sub = NULL;
	    if ( !aselection || (mv->perchar[j].sc->possub==NULL && mv->perchar[j].basesc==NULL) )
		vwlist[i].ti.disabled = true;
	    else {
		vwlist[i].sub = SubsMenuBuild(mv->perchar[j].sc,mv->perchar[j].basesc);
		vwlist[i].ti.disabled = (vwlist[i].sub == NULL);
	    }
	  break;
	  case MID_Vertical:
	    vwlist[i].ti.checked = mv->vertical;
	    vwlist[i].ti.disabled = !mv->fv->sf->hasvmetrics;
	  break;
	}
    base = i+1;
    for ( i=base; vwlist[i].ti.text!=NULL; ++i ) {
	free( vwlist[i].ti.text);
	vwlist[i].ti.text = NULL;
    }

    vwlist[base-1].ti.checked = mv->bdf==NULL;
    if ( mv->fv->sf->bitmaps!=NULL ) {
	for ( bdf = mv->fv->sf->bitmaps, i=base;
		i<sizeof(vwlist)/sizeof(vwlist[0])-1 && bdf!=NULL;
		++i, bdf = bdf->next ) {
	    if ( BDFDepth(bdf)==1 )
		sprintf( buffer, _("%d pixel bitmap"), bdf->pixelsize );
	    else
		sprintf( buffer, _("%d@%d pixel bitmap"),
			bdf->pixelsize, BDFDepth(bdf) );
	    vwlist[i].ti.text = utf82u_copy(buffer);
	    vwlist[i].ti.checkable = true;
	    vwlist[i].ti.checked = bdf==mv->bdf;
	    vwlist[i].ti.userdata = bdf;
	    vwlist[i].invoke = MVMenuShowBitmap;
	    vwlist[i].ti.fg = vwlist[i].ti.bg = COLOR_DEFAULT;
	}
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItemArrayCopy(vwlist,NULL);
}

static void mtlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_VKernClass:
	  case MID_VKernFromHKern:
	    mi->ti.disabled = !mv->fv->sf->hasvmetrics;
	  break;
	}
    }
}

static GMenuItem mblist[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, 0, 0, edlist, edlistcheck },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, 0, 0, ellist, ellistcheck },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 0, 0, mtlist, mtlistcheck },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, 0, 0, wnmenu, MVWindowMenuBuild, NULL },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};

static void MVResize(MetricsView *mv,GEvent *event) {
    GRect pos;
    int i;
    int size;

    if ( event->u.resize.size.height < mv->topend+20 + mv->height-mv->displayend ||
	    event->u.resize.size.width < 30 ) {
	int width= event->u.resize.size.width < 30 ? 30 : event->u.resize.size.width;
	int height;

	if ( event->u.resize.size.height < mv->topend+20 + mv->height-mv->displayend )
	    height = mv->topend+20 + mv->height-mv->displayend;
	else
	    height = event->u.resize.size.height;
	GDrawResize(mv->gw,width,height);
return;
    }

    mv->width = event->u.resize.size.width;
    mv->displayend = event->u.resize.size.height - (mv->height-mv->displayend);
    mv->height = event->u.resize.size.height;

    pos.width = event->u.resize.size.width;
    pos.height = mv->sbh;
    pos.y = event->u.resize.size.height - pos.height; pos.x = 0;
    GGadgetResize(mv->hsb,pos.width,pos.height);
    GGadgetMove(mv->hsb,pos.x,pos.y);

    mv->dwidth = mv->width - mv->sbh;
    GGadgetResize(mv->vsb,mv->sbh,mv->displayend-mv->topend);
    GGadgetMove(mv->vsb,event->u.resize.size.width-mv->sbh,mv->topend);

    size = (mv->displayend - mv->topend - 4);
    if ( mv->dwidth-20<size )
	size = mv->dwidth-20;
    mv->pixelsize = mv_scales[mv->scale_index]*size;

    for ( i=0; i<mv->max; ++i ) if ( mv->perchar[i].width!=NULL ) {
	GGadgetMove(mv->perchar[i].name,mv->perchar[i].mx,mv->displayend+2);
	GGadgetMove(mv->perchar[i].width,mv->perchar[i].mx,mv->displayend+2+mv->fh+4);
	GGadgetMove(mv->perchar[i].lbearing,mv->perchar[i].mx,mv->displayend+2+2*(mv->fh+4));
	GGadgetMove(mv->perchar[i].rbearing,mv->perchar[i].mx,mv->displayend+2+3*(mv->fh+4));
	if ( mv->perchar[i].kern!=NULL )
	    GGadgetMove(mv->perchar[i].kern,mv->perchar[i].mx-mv->perchar[i].mwidth/2,mv->displayend+2+4*(mv->fh+4));
	if ( i!=0 )
	    MVRefreshKern(mv,i);
    }
    MVSetAnchor(mv);
    GGadgetMove(mv->namelab,2,mv->displayend+2);
    GGadgetMove(mv->widthlab,2,mv->displayend+2+mv->fh+4);
    GGadgetMove(mv->lbearinglab,2,mv->displayend+2+2*(mv->fh+4));
    GGadgetMove(mv->rbearinglab,2,mv->displayend+2+3*(mv->fh+4));
    GGadgetMove(mv->kernlab,2,mv->displayend+2+4*(mv->fh+4));

    MVReRasterize(mv);
    MVSetVSb(mv);
    MVSetSb(mv);

    GDrawRequestExpose(mv->gw,NULL,true);
}

static void MVChar(MetricsView *mv,GEvent *event) {
    if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuSaveAll(NULL,NULL,NULL);
    else if ( event->u.chr.keysym=='I' &&
	    (event->u.chr.state&ksm_shift) &&
	    (event->u.chr.state&ksm_meta) )
	MVMenuCharInfo(mv->gw,NULL,NULL);
    else if ( event->u.chr.keysym == GK_Help ) {
	MenuHelp(NULL,NULL,NULL);	/* Menu does F1 */
    }
}

static int hitsbit(BDFChar *bc, int x, int y) {
    if ( bc->byte_data )
return( bc->bitmap[y*bc->bytes_per_line+x] );
    else
return( bc->bitmap[y*bc->bytes_per_line+(x>>3)]&(1<<(7-(x&7))) );
}

static struct aplist *hitsaps(MetricsView *mv,int i, int x,int y) {
    SplineFont *sf = mv->fv->sf;
    int emsize = sf->ascent+sf->descent;
    double scale = mv->pixelsize / (double) emsize;
    struct aplist *apl;
    int ax, ay;

    for ( apl = mv->perchar[i].aps; apl!=NULL; apl=apl->next ) {
	ax = apl->ap->me.x*scale;
	ay = apl->ap->me.y*scale;
	if ( x>ax-3 && x<ax+3   &&   y>ay-3 && y<ay+3 )
return( apl );
    }
return( NULL );
}

static void _MVVMouse(MetricsView *mv,GEvent *event) {
    int i, x, y, j, within, sel, xbase;
    SplineChar *sc;
    int diff;
    int onwidth, onkern;
    SplineFont *sf = mv->fv->sf;
    int as = rint(mv->pixelsize*sf->ascent/(double) (sf->ascent+sf->descent));
    double scale = mv->pixelsize/(double) (sf->ascent+sf->descent);

    xbase = mv->dwidth/2;
    within = -1;
    for ( i=0; i<mv->charcnt; ++i ) {
	y = mv->perchar[i].dy + mv->perchar[i].yoff;
	x = xbase - mv->pixelsize/2 - mv->perchar[i].xoff;
	if ( mv->bdf==NULL ) {
	    if ( event->u.mouse.x >= x+mv->perchar[i].show->xmin &&
		event->u.mouse.x <= x+mv->perchar[i].show->xmax &&
		event->u.mouse.y <= (y+as)-mv->perchar[i].show->ymin &&
		event->u.mouse.y >= (y+as)-mv->perchar[i].show->ymax &&
		hitsbit(mv->perchar[i].show,event->u.mouse.x-x-mv->perchar[i].show->xmin,
			event->u.mouse.y-(y+as-mv->perchar[i].show->ymax)) )
    break;
	}
	y += -mv->perchar[i].yoff;
	if ( event->u.mouse.y >= y && event->u.mouse.y < y+mv->perchar[i].dheight+ mv->perchar[i].kernafter+ mv->perchar[i].voff )
	    within = i;
    }
    if ( i==mv->charcnt )
	sc = NULL;
    else
	sc = mv->perchar[i].sc;

    diff = event->u.mouse.y-mv->pressed_y;
    sel = onwidth = onkern = false;
    if ( sc==NULL ) {
	if ( within>0 && mv->perchar[within-1].selected &&
		event->u.mouse.y<mv->perchar[within].dy+3 )
	    onwidth = true;		/* previous char */
	else if ( within!=-1 && within+1<mv->charcnt &&
		mv->perchar[within+1].selected &&
		event->u.mouse.y>mv->perchar[within+1].dy-3 )
	    onkern = true;			/* subsequent char */
	else if ( within>=0 && mv->perchar[within].selected &&
		event->u.mouse.y<mv->perchar[within].dy+3 )
	    onkern = true;
	else if ( within>=0 &&
		event->u.mouse.y>mv->perchar[within].dy+mv->perchar[within].dheight+mv->perchar[within].kernafter+mv->perchar[within].voff-3 ) {
	    onwidth = true;
	    sel = true;
	}
    }

    if ( event->type != et_mousemove || !mv->pressed ) {
	int ct = -1;
	if ( mv->bdf!=NULL ) {
	    if ( mv->cursor!=ct_mypointer )
		ct = ct_mypointer;
	} else if ( sc!=NULL ) {
	    if ( mv->cursor!=ct_lbearing )
		ct = ct_lbearing;
	} else if ( onwidth ) {
	    if ( mv->cursor!=ct_rbearing )
		ct = ct_rbearing;
	} else if ( onkern ) {
	    if ( mv->cursor!=ct_kerning )
		ct = ct_kerning;
	} else {
	    if ( mv->cursor!=ct_mypointer )
		ct = ct_mypointer;
	}
	if ( ct!=-1 ) {
	    GDrawSetCursor(mv->gw,ct);
	    mv->cursor = ct;
	}
    }

    if ( event->type == et_mousemove && !mv->pressed ) {
	if ( sc==NULL && within!=-1 )
	    sc = mv->perchar[within].sc;
	if ( sc!=NULL ) 
	    SCPreparePopup(mv->gw,sc,mv->fv->map->remap,mv->fv->map->backmap[sc->orig_pos],sc->unicodeenc);
/* Don't allow any editing when displaying a bitmap font */
    } else if ( event->type == et_mousedown && mv->bdf==NULL ) {
	CVPaletteDeactivate();
	if ( sc!=NULL ) {
	    for ( j=0; j<mv->charcnt; ++j )
		if ( j!=i && mv->perchar[j].selected )
		    MVDeselectChar(mv,j);
	    MVSelectChar(mv,i);
	    GWindowClearFocusGadgetOfWindow(mv->gw);
	    if ( event->u.mouse.button==3 )
		MVPopupMenu(mv,event,i);
	    else
		mv->pressed = true;
	} else if ( within!=-1 ) {
	    mv->pressedwidth = onwidth;
	    mv->pressedkern = onkern;
	    if ( mv->pressedwidth || mv->pressedkern ) {
		mv->pressed = true;
		if ( sel && !mv->perchar[within].selected ) {
		    MVDoSelect(mv,within);
		}
	    }
	} else if ( event->u.mouse.y<mv->perchar[mv->charcnt-1].dy+
				    mv->perchar[mv->charcnt-1].dheight+
			            mv->perchar[mv->charcnt-1].kernafter+
			            mv->perchar[mv->charcnt-1].voff+3 ) {
	    mv->pressed = mv->pressedwidth = true;
	    GDrawSetCursor(mv->gw,ct_rbearing);
	    mv->cursor = ct_rbearing;
	    if ( !mv->perchar[mv->charcnt-1].selected )
		    MVDoSelect(mv,mv->charcnt-1);
	}
	mv->pressed_y = event->u.mouse.y;
    } else if ( event->type == et_mousemove && mv->pressed ) {
	for ( i=0; i<mv->charcnt && !mv->perchar[i].selected; ++i );
	if ( mv->pressedwidth ) {
	    int ow = mv->perchar[i].dwidth;
	    mv->perchar[i].dwidth = rint(mv->perchar[i].sc->vwidth*scale) + diff;
	    if ( ow!=mv->perchar[i].dwidth ) {
		for ( j=i+1; j<mv->charcnt; ++j )
		    mv->perchar[j].dy = mv->perchar[j-1].dy+mv->perchar[j-1].dheight+
			    mv->perchar[j-1].kernafter+ mv->perchar[j-1].voff;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    int ow = mv->perchar[i-1].kernafter;
	    KernPair *kp;
	    int kpoff;
	    KernClass *kc;
	    int index;
	    for ( kp = mv->perchar[i-1].sc->vkerns; kp!=NULL && kp->sc!=mv->perchar[i].sc; kp = kp->next );
	    if ( kp!=NULL )
		kpoff = kp->off;
	    else if ((kc=SFFindVKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,false))!=NULL )
		kpoff = kc->offsets[index];
	    else
		kpoff = 0;
	    kpoff = kpoff * mv->pixelsize /
			(mv->fv->sf->descent+mv->fv->sf->ascent);
	    mv->perchar[i-1].kernafter = kpoff + diff;
	    if ( ow!=mv->perchar[i-1].kernafter ) {
		for ( j=i; j<mv->charcnt; ++j )
		    mv->perchar[j].dy = mv->perchar[j-1].dy+mv->perchar[j-1].dheight+
			    mv->perchar[j-1].kernafter+ mv->perchar[j-1].voff;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else {
	    int olda = mv->activeoff;
	    mv->activeoff = diff;
	    MVRedrawI(mv,i,mv->perchar[i].show->xmin+olda,mv->perchar[i].show->xmax+olda);
	}
    } else if ( event->type == et_mouseup && event->u.mouse.clicks>1 &&
	    (within!=-1 || sc!=NULL)) {
	mv->pressed = false; mv->activeoff = 0;
	mv->pressedwidth = mv->pressedkern = false;
	if ( within==-1 ) within = i;
	if ( mv->bdf==NULL )
	    CharViewCreate(mv->perchar[within].sc,mv->fv,-1);
	else
	    BitmapViewCreate(mv->bdf->glyphs[mv->perchar[within].sc->orig_pos],mv->bdf,mv->fv,-1);
    } else if ( event->type == et_mouseup && mv->pressed ) {
	for ( i=0; i<mv->charcnt && !mv->perchar[i].selected; ++i );
	mv->pressed = false;
	mv->activeoff = 0;
	sc = mv->perchar[i].sc;
	if ( mv->pressedwidth ) {
	    mv->pressedwidth = false;
	    diff = diff*(mv->fv->sf->ascent+mv->fv->sf->descent)/mv->pixelsize;
	    if ( diff!=0 ) {
		SCPreserveWidth(sc);
		sc->vwidth += diff;
		SCCharChangedUpdate(sc);
		for ( ; i<mv->charcnt; ++i )
		    mv->perchar[i].dy = mv->perchar[i-1].dy+mv->perchar[i-1].dheight +
			    mv->perchar[i-1].kernafter + mv->perchar[i-1].voff;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    mv->pressedkern = false;
	    diff = diff/scale;
	    if ( diff!=0 ) {
		KernPair *kp;
		KernClass *kc=NULL;
		int index;
		for ( kp=mv->perchar[i-1].sc->vkerns; kp!=NULL && kp->sc!=mv->perchar[i].sc; kp = kp->next );
		if ( kp==NULL )
		    kc=SFFindVKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,true);
		if ( kc!=NULL ) {
		    if ( kc->offsets[index]==0 && !AskNewKernClassEntry(mv->perchar[i-1].sc,mv->perchar[i].sc))
			kc=NULL;
		    else
			kc->offsets[index] += diff;
		}
		if ( kc==NULL ) {
		    if ( kp==NULL ) {
			kp = chunkalloc(sizeof(KernPair));
			kp->sc = mv->perchar[i].sc;
			kp->next = mv->perchar[i-1].sc->vkerns;
			mv->perchar[i-1].sc->vkerns = kp;
		    }
		    kp->off += diff;
		    kp->sli = mv->cur_sli;
		}
		mv->perchar[i-1].kernafter = (kp==NULL?kc->offsets[index]:kp->off)*scale;
		MVRefreshValues(mv,i-1,mv->perchar[i-1].sc);
		for ( ; i<mv->charcnt; ++i )
		    mv->perchar[i].dy = mv->perchar[i-1].dy+mv->perchar[i-1].dheight +
			    mv->perchar[i-1].kernafter + mv->perchar[i-1].voff;
		MVSetAnchor(mv);
		GDrawRequestExpose(mv->gw,NULL,false);
		mv->fv->sf->changed = true;
	    }
	} else {
	    real transform[6];
	    DBounds bb;
	    SplineCharFindBounds(sc,&bb);
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[4] = 0;
	    transform[5] = -diff/scale;
	    if ( transform[5]!=0 )
		FVTrans(mv->fv,sc,transform,NULL,false);
	}
    } else if ( event->type == et_mouseup && mv->bdf!=NULL && within!=-1 ) {
	for ( j=0; j<mv->charcnt; ++j )
	    if ( j!=within && mv->perchar[j].selected )
		MVDeselectChar(mv,j);
	MVSelectChar(mv,within);
    }
}

static void MVMouse(MetricsView *mv,GEvent *event) {
    int i, x, y, j, within, sel, ybase;
    SplineChar *sc;
    struct aplist *apl=NULL;
    int diff;
    int onwidth, onkern;

    GGadgetEndPopup();
    if ( event->u.mouse.y< mv->topend || event->u.mouse.y >= mv->displayend ) {
	if ( event->u.mouse.y >= mv->displayend &&
		event->u.mouse.y<mv->height-mv->sbh ) {
	    event->u.mouse.x += (mv->coff*mv->mwidth);
	    for ( i=0; i<mv->charcnt; ++i ) {
		if ( event->u.mouse.x >= mv->perchar[i].mx &&
			event->u.mouse.x < mv->perchar[i].mx+mv->perchar[i].mwidth )
	    break;
	    }
	    if ( i<mv->charcnt )
		SCPreparePopup(mv->gw,mv->perchar[i].sc,mv->fv->map->remap,
			mv->fv->map->backmap[mv->perchar[i].sc->orig_pos],
			mv->perchar[i].sc->unicodeenc);
	}
	if ( mv->cursor!=ct_mypointer ) {
	    GDrawSetCursor(mv->gw,ct_mypointer);
	    mv->cursor = ct_mypointer;
	}
return;
    }

    if ( event->type==et_mouseup ) {
	event->type = et_mousemove;
	MVMouse(mv,event);
	event->u.mouse.x -= mv->xoff;
	event->u.mouse.y -= mv->yoff;
	event->type = et_mouseup;
    }

    event->u.mouse.x += mv->xoff;
    event->u.mouse.y += mv->yoff;
    if ( mv->vertical ) {
	_MVVMouse(mv,event);
return;
    }

    ybase = mv->topend + 2 + (mv->pixelsize/mv_scales[mv->scale_index] * mv->fv->sf->ascent /
	    (mv->fv->sf->ascent+mv->fv->sf->descent));
    within = -1;
    for ( i=0; i<mv->charcnt; ++i ) {
	x = mv->perchar[i].dx + mv->perchar[i].xoff;
	if ( mv->right_to_left )
	    x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter - mv->perchar[i].hoff;
	y = ybase - mv->perchar[i].yoff;
	if ( mv->bdf==NULL ) {
	    if ( mv->perchar[i].selected && mv->perchar[i].aps!=NULL &&
		(apl=hitsaps(mv,i,event->u.mouse.x-x,y-event->u.mouse.y))!=NULL )
    break;
	    if ( event->u.mouse.x >= x+mv->perchar[i].show->xmin &&
		event->u.mouse.x <= x+mv->perchar[i].show->xmax &&
		event->u.mouse.y <= y-mv->perchar[i].show->ymin &&
		event->u.mouse.y >= y-mv->perchar[i].show->ymax &&
		hitsbit(mv->perchar[i].show,event->u.mouse.x-x-mv->perchar[i].show->xmin,
			mv->perchar[i].show->ymax-(y-event->u.mouse.y)) )
    break;
	}
	x += mv->right_to_left ? mv->perchar[i].xoff : -mv->perchar[i].xoff;
	if ( event->u.mouse.x >= x && event->u.mouse.x < x+mv->perchar[i].dwidth+ mv->perchar[i].kernafter+ mv->perchar[i].hoff )
	    within = i;
    }
    if ( i==mv->charcnt )
	sc = NULL;
    else
	sc = mv->perchar[i].sc;

    diff = event->u.mouse.x-mv->pressed_x;
    /*if ( mv->right_to_left ) diff = -diff;*/
    sel = onwidth = onkern = false;
    if ( sc==NULL && apl==NULL ) {
	if ( !mv->right_to_left ) {
	    if ( within>0 && mv->perchar[within-1].selected &&
		    event->u.mouse.x<mv->perchar[within].dx+3 )
		onwidth = true;		/* previous char */
	    else if ( within!=-1 && within+1<mv->charcnt &&
		    mv->perchar[within+1].selected &&
		    event->u.mouse.x>mv->perchar[within+1].dx-3 )
		onkern = true;			/* subsequent char */
	    else if ( within>=0 && mv->perchar[within].selected &&
		    event->u.mouse.x<mv->perchar[within].dx+3 )
		onkern = true;
	    else if ( within>=0 &&
		    event->u.mouse.x>mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter+mv->perchar[within].hoff-3 ) {
		onwidth = true;
		sel = true;
	    }
	} else {
	    if ( within>0 && mv->perchar[within-1].selected &&
		    event->u.mouse.x>mv->dwidth-(mv->perchar[within].dx+3) )
		onwidth = true;		/* previous char */
	    else if ( within!=-1 && within+1<mv->charcnt &&
		    mv->perchar[within+1].selected &&
		    event->u.mouse.x<mv->dwidth-(mv->perchar[within+1].dx-3) )
		onkern = true;			/* subsequent char */
	    else if ( within>=0 && mv->perchar[within].selected &&
		    event->u.mouse.x>mv->dwidth-(mv->perchar[within].dx+3) )
		onkern = true;
	    else if ( within>=0 &&
		    event->u.mouse.x<mv->dwidth-(mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter+mv->perchar[within].hoff-3) ) {
		onwidth = true;
		sel = true;
	    }
	}
    }

    if ( event->type != et_mousemove || !mv->pressed ) {
	int ct = -1;
	if ( mv->bdf!=NULL ) {
	    if ( mv->cursor!=ct_mypointer )
		ct = ct_mypointer;
	} else if ( apl!=NULL ) {
	    if ( mv->cursor!=ct_4way )
		ct = ct_4way;
	} else if ( sc!=NULL ) {
	    if ( mv->cursor!=ct_lbearing )
		ct = ct_lbearing;
	} else if ( onwidth ) {
	    if ( mv->cursor!=ct_rbearing )
		ct = ct_rbearing;
	} else if ( onkern ) {
	    if ( mv->cursor!=ct_kerning )
		ct = ct_kerning;
	} else {
	    if ( mv->cursor!=ct_mypointer )
		ct = ct_mypointer;
	}
	if ( ct!=-1 ) {
	    GDrawSetCursor(mv->gw,ct);
	    mv->cursor = ct;
	}
    }

    if ( event->type == et_mousemove && !mv->pressed ) {
	if ( sc==NULL && within!=-1 )
	    sc = mv->perchar[within].sc;
	if ( sc!=NULL ) 
	    SCPreparePopup(mv->gw,sc,mv->fv->map->remap,mv->fv->map->backmap[sc->orig_pos],sc->unicodeenc);
/* Don't allow any editing when displaying a bitmap font */
    } else if ( event->type == et_mousedown && mv->bdf==NULL ) {
	CVPaletteDeactivate();
	if ( apl!=NULL ) {
	    mv->pressed_apl = apl;
	    apl->selected = true;
	    mv->pressed = true;
	    mv->ap_owner = i;
	    mv->xp = event->u.mouse.x; mv->yp = event->u.mouse.y;
	    mv->ap_start = apl->ap->me;
	    MVRedrawI(mv,i,0,0);
	    SCPreserveState(mv->perchar[i].sc,false);
	    GWindowClearFocusGadgetOfWindow(mv->gw);
	} else if ( sc!=NULL ) {
	    for ( j=0; j<mv->charcnt; ++j )
		if ( j!=i && mv->perchar[j].selected )
		    MVDeselectChar(mv,j);
	    MVSelectChar(mv,i);
	    GWindowClearFocusGadgetOfWindow(mv->gw);
	    if ( event->u.mouse.button==3 )
		MVPopupMenu(mv,event,i);
	    else
		mv->pressed = true;
	} else if ( within!=-1 ) {
	    mv->pressedwidth = onwidth;
	    mv->pressedkern = onkern;
	    if ( mv->pressedwidth || mv->pressedkern ) {
		mv->pressed = true;
		if ( sel && !mv->perchar[within].selected ) {
		    MVDoSelect(mv,within);
		}
	    }
	} else if ( !mv->right_to_left && mv->charcnt>=1 &&
		event->u.mouse.x<mv->perchar[mv->charcnt-1].dx+mv->perchar[mv->charcnt-1].dwidth+mv->perchar[mv->charcnt-1].kernafter+mv->perchar[mv->charcnt-1].hoff+3 ) {
	    mv->pressed = mv->pressedwidth = true;
	    GDrawSetCursor(mv->gw,ct_rbearing);
	    mv->cursor = ct_rbearing;
	    if ( !mv->perchar[mv->charcnt-1].selected )
		    MVDoSelect(mv,mv->charcnt-1);
	} else if ( mv->right_to_left && mv->charcnt>=1 &&
		event->u.mouse.x>mv->dwidth - (mv->perchar[mv->charcnt-1].dx+mv->perchar[mv->charcnt-1].dwidth+mv->perchar[mv->charcnt-1].kernafter+mv->perchar[mv->charcnt-1].hoff+3) ) {
	    mv->pressed = mv->pressedwidth = true;
	    GDrawSetCursor(mv->gw,ct_rbearing);
	    mv->cursor = ct_rbearing;
	    if ( !mv->perchar[mv->charcnt-1].selected )
		    MVDoSelect(mv,mv->charcnt-1);
	}
	mv->pressed_x = event->u.mouse.x;
    } else if ( event->type == et_mousemove && mv->pressed ) {
	for ( i=0; i<mv->charcnt && !mv->perchar[i].selected; ++i );
	if ( mv->pressed_apl ) {
	    double scale = mv->pixelsize/(double) (mv->fv->sf->ascent+mv->fv->sf->descent);
	    mv->pressed_apl->ap->me.x = mv->ap_start.x + (event->u.mouse.x-mv->xp)/scale;
	    mv->pressed_apl->ap->me.y = mv->ap_start.y + (mv->yp-event->u.mouse.y)/scale;
	    MVSetAnchor(mv);
	    MVRedrawI(mv,mv->ap_owner,0,0);
	} else if ( mv->pressedwidth ) {
	    int ow = mv->perchar[i].dwidth;
	    if ( mv->right_to_left ) diff = -diff;
	    mv->perchar[i].dwidth = mv->perchar[i].show->width + diff;
	    if ( ow!=mv->perchar[i].dwidth ) {
		for ( j=i+1; j<mv->charcnt; ++j )
		    mv->perchar[j].dx = mv->perchar[j-1].dx+mv->perchar[j-1].dwidth+ mv->perchar[j-1].kernafter+ mv->perchar[j-1].hoff;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    int ow = mv->perchar[i-1].kernafter;
	    KernPair *kp;
	    int kpoff;
	    KernClass *kc;
	    int index;
	    for ( kp = mv->perchar[i-1].sc->kerns; kp!=NULL && kp->sc!=mv->perchar[i].sc; kp = kp->next );
	    if ( kp!=NULL )
		kpoff = kp->off;
	    else if ((kc=SFFindKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,false))!=NULL )
		kpoff = kc->offsets[index];
	    else
		kpoff = 0;
	    kpoff = kpoff * mv->pixelsize /
			(mv->fv->sf->descent+mv->fv->sf->ascent);
	    if ( mv->right_to_left ) diff = -diff;
	    mv->perchar[i-1].kernafter = kpoff + diff;
	    if ( ow!=mv->perchar[i-1].kernafter ) {
		for ( j=i; j<mv->charcnt; ++j )
		    mv->perchar[j].dx = mv->perchar[j-1].dx+mv->perchar[j-1].dwidth+ mv->perchar[j-1].kernafter+ mv->perchar[j-1].hoff;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else {
	    int olda = mv->activeoff;
	    mv->activeoff = diff;
	    MVRedrawI(mv,i,mv->perchar[i].show->xmin+olda,mv->perchar[i].show->xmax+olda);
	}
    } else if ( event->type == et_mouseup && event->u.mouse.clicks>1 &&
	    (within!=-1 || sc!=NULL)) {
	mv->pressed = false; mv->activeoff = 0;
	mv->pressedwidth = mv->pressedkern = false;
	if ( mv->pressed_apl!=NULL ) {
	    mv->pressed_apl->selected = false;
	    mv->pressed_apl = NULL;
	}
	if ( within==-1 ) within = i;
	if ( mv->bdf==NULL )
	    CharViewCreate(mv->perchar[within].sc,mv->fv,-1);
	else
	    BitmapViewCreate(mv->bdf->glyphs[mv->perchar[within].sc->orig_pos],mv->bdf,mv->fv,-1);
    } else if ( event->type == et_mouseup && mv->pressed ) {
	for ( i=0; i<mv->charcnt && !mv->perchar[i].selected; ++i );
	mv->pressed = false;
	mv->activeoff = 0;
	sc = mv->perchar[i].sc;
	if ( mv->pressed_apl!=NULL ) {
	    mv->pressed_apl->selected = false;
	    mv->pressed_apl = NULL;
	    MVRedrawI(mv,mv->ap_owner,0,0);
	    SCCharChangedUpdate(mv->perchar[mv->ap_owner].sc);
	} else if ( mv->pressedwidth ) {
	    mv->pressedwidth = false;
	    if ( mv->right_to_left ) diff = -diff;
	    diff = diff*(mv->fv->sf->ascent+mv->fv->sf->descent)/mv->pixelsize;
	    if ( diff!=0 ) {
		SCPreserveWidth(sc);
		sc->width += diff;
		SCCharChangedUpdate(sc);
	    }
	} else if ( mv->pressedkern ) {
	    mv->pressedkern = false;
	    diff = diff*(mv->fv->sf->ascent+mv->fv->sf->descent)/mv->pixelsize;
	    if ( diff!=0 ) {
		KernPair *kp;
		KernClass *kc=NULL;
		int index;
		for ( kp= mv->perchar[i-1].sc->kerns; kp!=NULL && kp->sc!=mv->perchar[i].sc; kp = kp->next );
		if ( kp==NULL )
		    kc= SFFindKernClass(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,&index,true);
		if ( mv->right_to_left ) diff = -diff;
		if ( kc!=NULL ) {
		    if ( kc->offsets[index]==0 && !AskNewKernClassEntry(mv->perchar[i-1].sc,mv->perchar[i].sc))
			kc=NULL;
		    else
			kc->offsets[index] += diff;
		}
		if ( kc==NULL ) {
		    MMKern(mv->fv->sf,mv->perchar[i-1].sc,mv->perchar[i].sc,
			    diff,mv->cur_sli,kp);
		    if ( kp==NULL ) {
			kp = chunkalloc(sizeof(KernPair));
			kp->sc = mv->perchar[i].sc;
			kp->next = mv->perchar[i-1].sc->kerns;
			mv->perchar[i-1].sc->kerns = kp;
		    }
		    kp->off += diff;
		    kp->sli = mv->cur_sli;
		}
		mv->perchar[i-1].kernafter = (kp==NULL?kc->offsets[index]:kp->off)*mv->pixelsize/
			(mv->fv->sf->ascent+mv->fv->sf->descent);
		MVRefreshValues(mv,i-1,mv->perchar[i-1].sc);
		for ( ; i<mv->charcnt; ++i )
		    mv->perchar[i].dx = mv->perchar[i-1].dx+mv->perchar[i-1].dwidth +
			    mv->perchar[i-1].kernafter + mv->perchar[i-1].hoff;
		MVSetAnchor(mv);
		GDrawRequestExpose(mv->gw,NULL,false);
		mv->fv->sf->changed = true;
	    }
	} else {
	    real transform[6];
	    DBounds bb;
	    SplineCharFindBounds(sc,&bb);
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[5] = 0;
	    transform[4] = diff*
		    (mv->fv->sf->ascent+mv->fv->sf->descent)/mv->pixelsize;
	    if ( transform[4]!=0 )
		FVTrans(mv->fv,sc,transform,NULL,false);
	}
    } else if ( event->type == et_mouseup && mv->bdf!=NULL && within!=-1 ) {
	if ( mv->pressed_apl!=NULL ) {
	    mv->pressed_apl->selected = false;
	    mv->pressed_apl = NULL;
	}
	for ( j=0; j<mv->charcnt; ++j )
	    if ( j!=within && mv->perchar[j].selected )
		MVDeselectChar(mv,j);
	MVSelectChar(mv,within);
    }
}

static void MVDrop(MetricsView *mv,GEvent *event) {
    int x,ex = event->u.drag_drop.x + mv->xoff;
    int y,ey = event->u.drag_drop.y + mv->yoff;
    int within, i, cnt, ch;
    int32 len;
    char *cnames, *start, *pt;
    unichar_t *newtext;
    const unichar_t *oldtext;
    SplineChar **founds;
    /* We should get a list of character names. Add them before the character */
    /*  on which they are dropped */

    if ( !GDrawSelectionHasType(mv->gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(mv->gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    within = mv->charcnt;
    if ( !mv->vertical ) {
	for ( i=0; i<mv->charcnt; ++i ) {
	    x = mv->perchar[i].dx;
	    if ( mv->right_to_left )
		x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter - mv->perchar[i].hoff;
	    if ( ex >= x && ex < x+mv->perchar[i].dwidth+ mv->perchar[i].kernafter+ mv->perchar[i].hoff ) {
		within = i;
	break;
	    }
	}
    } else {
	for ( i=0; i<mv->charcnt; ++i ) {
	    y = mv->perchar[i].dy;
	    if ( ey >= y && ey < y+mv->perchar[i].dheight+
		    mv->perchar[i].kernafter+ mv->perchar[i].voff ) {
		within = i;
	break;
	    }
	}
    }

    founds = galloc(len*sizeof(SplineChar *));	/* Will be a vast over-estimate */
    start = cnames;
    for ( i=0; *start; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	if ( (founds[i]=SFGetChar(mv->fv->sf,-1,start))!=NULL )
	    ++i;
	*pt = ch;
	start = pt;
    }
    cnt = i;
    free( cnames );
    if ( cnt==0 )
return;

    if ( mv->charcnt+cnt>=mv->max ) {
	int oldmax=mv->max;
	mv->max = mv->charcnt+cnt+10;
	mv->perchar = grealloc(mv->perchar,mv->max*sizeof(struct metricchar));
	mv->sstr = grealloc(mv->sstr,(mv->max+1)*sizeof(SplineChar *));
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }
    oldtext = _GGadgetGetTitle(mv->text);
    newtext = galloc((mv->charcnt+cnt+1)*sizeof(unichar_t));
    u_strcpy(newtext,oldtext);
    newtext[mv->charcnt+cnt]='\0';
    for ( i=mv->charcnt+cnt-1; i>=within+cnt; --i ) {
	newtext[i] = newtext[i-cnt];
	mv->perchar[i].sc = mv->perchar[i-cnt].sc;
	mv->perchar[i].active_pos = mv->perchar[i-cnt].active_pos;
	mv->perchar[i].show = mv->perchar[i-cnt].show;
	mv->perchar[i-cnt].show = NULL;
    }
    for ( i=within; i<within+cnt; ++i ) {
	mv->perchar[i].sc = founds[i-within];
	mv->perchar[i].active_pos = NULL;
	newtext[i] = (founds[i-within]->unicodeenc>=0 && founds[i-within]->unicodeenc<0x10000)?
		founds[i-within]->unicodeenc : 0xfffd;
    }
    mv->charcnt += cnt;
    for ( i=within; i<mv->charcnt; ++i )
	MVSetPos(mv,i,mv->perchar[i].sc);
    MVSetAnchor(mv);
    free(founds);

    GGadgetSetTitle(mv->text,newtext);
    free(newtext);

    GDrawRequestExpose(mv->gw,NULL,false);
    MVSetSb(mv);
}

static int mv_e_h(GWindow gw, GEvent *event) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_selclear:
	ClipboardClear();
      break;
      case et_expose:
	GDrawSetLineWidth(gw,0);
	MVExpose(mv,gw,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    MVResize(mv,event);
      break;
      case et_char:
	MVChar(mv,event);
      break;
      case et_mouseup: case et_mousemove: case et_mousedown:
	if (( event->type==et_mouseup || event->type==et_mousedown ) &&
		(event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(mv->vsb,event));
	}
	MVMouse(mv,event);
      break;
      case et_drop:
	MVDrop(mv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g==mv->hsb )
		MVHScroll(mv,&event->u.control.u.sb);
	    else
		MVVScroll(mv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	MVMenuClose(gw,NULL,NULL);
      break;
      case et_destroy:
	if ( mv->fv->metrics==mv )
	    mv->fv->metrics = mv->next;
	else {
	    MetricsView *n;
	    for ( n=mv->fv->metrics; n->next!=mv; n=n->next );
	    n->next = mv->next;
	}
	KCLD_MvDetach(mv->fv->sf->kcld,mv);
	MetricsViewFree(mv);
      break;
      case et_focus:
#if 0
	if ( event->u.focus.gained_focus )
	    CVPaletteDeactivate();
#endif
      break;
    }
return( true );
}

#define metricsicon_width 16
#define metricsicon_height 16
static unsigned char metricsicon_bits[] = {
   0x04, 0x10, 0xf0, 0x03, 0x24, 0x12, 0x20, 0x00, 0x24, 0x10, 0xe0, 0x00,
   0x24, 0x10, 0x20, 0x00, 0x24, 0x10, 0x20, 0x00, 0x74, 0x10, 0x00, 0x00,
   0x55, 0x55, 0x00, 0x00, 0x04, 0x10, 0x00, 0x00};

static void MetricsViewInit(void ) {
    mbDoGetText(mblist);
}

MetricsView *MetricsViewCreate(FontView *fv,SplineChar *sc,BDFFont *bdf) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GRect gsize;
    MetricsView *mv = gcalloc(1,sizeof(MetricsView));
    FontRequest rq;
    static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a', ',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    static GWindow icon = NULL;
    extern int _GScrollBar_Width;
    char buf[100], *pt;
    GTextInfo label;
    int i,j,cnt;
    int as,ds,ld;
    SplineChar *anysc, *goodsc;

    MetricsViewInit();

    if ( icon==NULL )
	icon = GDrawCreateBitmap(NULL,metricsicon_width,metricsicon_height,metricsicon_bits);

    mv->fv = fv;
    mv->bdf = bdf;
    mv->showgrid = true;
    mv->antialias = mv_antialias;
    mv->scale_index = 2;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_mypointer;
    snprintf(buf,sizeof(buf)/sizeof(buf[0]),
	    _("Metrics For %.50s"), fv->sf->fontname);
    wattrs.utf8_window_title = buf;
    wattrs.icon = icon;
    pos.x = pos.y = 0;
    pos.width = 800;
    pos.height = 300;
    mv->gw = gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,mv,&wattrs);
    mv->width = pos.width; mv->height = pos.height;

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = MVMenuContextualHelp;
    gd.u.menu = mblist;
    mv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(mv->mb,&gsize);
    mv->mbh = gsize.height;

    gd.pos.height = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.y = pos.height-gd.pos.height;
    gd.pos.x = 0; gd.pos.width = pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    mv->hsb = GScrollBarCreate(gw,&gd,mv);
    GGadgetGetSize(mv->hsb,&gsize);
    mv->sbh = gsize.height;
    mv->dwidth = mv->width-mv->sbh;

    gd.pos.width = mv->sbh;
    gd.pos.y = 0; gd.pos.height = pos.height;	/* we'll fix these later */
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    mv->vsb = GScrollBarCreate(gw,&gd,mv);

    memset(&rq,0,sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    mv->font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(mv->font,&as,&ds,&ld);
    mv->fh = as+ds; mv->as = as;

    pt = buf;
    mv->perchar = gcalloc(mv->max=20,sizeof(struct metricchar));
    mv->sstr = galloc(mv->max*sizeof(SplineChar *));
    if ( sc!=NULL ) {
	pt = utf8_idpb(pt,sc->unicodeenc==-1 ||sc->unicodeenc>=0x10000 ? 0xfffd: sc->unicodeenc);
	mv->perchar[0].sc = sc;
	cnt = 1;
    } else {
	EncMap *map = fv->map;
	for ( cnt=0, j=1; (j<=fv->sel_index || j<1) && cnt<15; ++j ) {
	    for ( i=0; i<map->enccount && cnt<15; ++i ) {
		int gid = map->map[i];
		if ( gid!=-1 && fv->selected[i]==j && fv->sf->glyphs[gid]!=NULL ) {
		    mv->perchar[cnt++].sc = fv->sf->glyphs[gid];
		    pt = utf8_idpb(pt,
			    fv->sf->glyphs[gid]->unicodeenc==-1 || fv->sf->glyphs[gid]->unicodeenc>=0x10000?
			    0xfffd: fv->sf->glyphs[gid]->unicodeenc);
		}
	    }
	}
    }
    *pt = '\0';
    if ( cnt!=0 ) {
	MVSelectChar(mv,cnt-1);
	pt = buf;
	mv->right_to_left = isrighttoleft(utf8_ildb((const char **) &pt))?1:0;
    }
    mv->charcnt = cnt;

    memset(&gd,0,sizeof(gd));
    memset(&label,0,sizeof(label));
    gd.pos.y = mv->mbh+2; gd.pos.x = 10;
    gd.pos.width = GDrawPointsToPixels(mv->gw,200);
    gd.label = &label;
    label.text = (unichar_t *) buf;
    label.text_is_1byte = true;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_text_xim;
    gd.handle_controlevent = MV_TextChanged;
    mv->text = GTextFieldCreate(gw,&gd,mv);
    GGadgetGetSize(mv->text,&gsize);
    mv->topend = gsize.y + gsize.height + 2;

    anysc = goodsc = NULL;
    for ( i=0; i<cnt; ++i ) {
	if ( anysc==NULL ) anysc = mv->perchar[i].sc;
	if ( SCScriptFromUnicode(mv->perchar[i].sc)!=DEFAULT_SCRIPT ) {
	    goodsc = mv->perchar[i].sc;
    break;
	}
    }
    if ( goodsc==NULL ) {
	SplineFont *sf = mv->fv->sf;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    if ( anysc==NULL ) anysc = sf->glyphs[i];
	    if ( SCScriptFromUnicode(sf->glyphs[i])!=DEFAULT_SCRIPT ) {
		goodsc = sf->glyphs[i];
	break;
	    }
	}
    }
    if ( goodsc==NULL ) goodsc = anysc;

    gd.pos.x = gd.pos.x+gd.pos.width+10; --gd.pos.y;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gd.handle_controlevent = MV_SLIChanged;
    gd.u.list = SFLangList(mv->fv->sf,true,goodsc);
    gd.label = NULL;
    mv->sli_list = GListButtonCreate(gw,&gd,mv);

    MVMakeLabels(mv);

    GGadgetMove(mv->vsb,mv->dwidth,mv->topend);
    GGadgetResize(mv->vsb,mv->sbh,mv->displayend-mv->topend);

    if ( sc!=NULL ) {
	MVSetPos(mv,0,sc);
    } else {
	EncMap *map = fv->map;
	for ( cnt=0, j=1; j<=fv->sel_index && cnt<15; ++j ) {
	    for ( i=0; i<fv->map->enccount && cnt<15; ++i ) {
		int gid = map->map[i];
		if ( gid!=-1 && fv->selected[i]==j && fv->sf->glyphs[gid]!=NULL ) {
		    MVSetPos(mv,cnt++,fv->sf->glyphs[gid]);
		}
	    }
	}
    }
    MVSetAnchor(mv);
    MVSetSb(mv);

    GDrawSetVisible(gw,true);
    /*GWidgetHidePalettes();*/
    mv->next = fv->metrics;
    fv->metrics = mv;
return( mv );
}

void MetricsViewFree(MetricsView *mv) {
    int i;

    for ( i=0; i<mv->charcnt; ++i )
	BDFCharFree(mv->perchar[i].show);
    /* the fields will free themselves */
    free(mv->perchar);
    free(mv);
}

void MVRefreshAll(MetricsView *mv) {
    int i;

    if ( mv!=NULL )
	for ( i=0; i<mv->charcnt; ++i )
	    MVSetPos(mv,i,mv->perchar[i].sc);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
