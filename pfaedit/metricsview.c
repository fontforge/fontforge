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
#include <gkeysym.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

static int mv_antialias = true;

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

    clip = &event->u.expose.rect;
    if ( clip->y+clip->height < mv->topend )
return;
    GDrawPushClip(pixmap,clip,&old);
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
    ybase = mv->topend + 2 + (mv->pixelsize * sf->ascent / (sf->ascent+sf->descent));
    if ( mv->showgrid )
	GDrawDrawLine(pixmap,0,ybase,mv->width,ybase,0x808080);

    r.x = clip->x; r.width = clip->width;
    r.y = mv->topend; r.height = mv->displayend-mv->topend;
    GDrawPushClip(pixmap,&r,&old2);
    if ( mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[0].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->width - x - mv->perchar[0].dwidth - mv->perchar[0].kernafter;
	GDrawDrawLine(pixmap,x,mv->topend,x,mv->displayend,0x808080);
    }
    si = -1;
    for ( i=0; i<mv->charcnt; ++i ) {
	if ( mv->perchar[i].selected ) si = i;
	x = mv->perchar[i].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->width - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
	if ( mv->bdf==NULL && mv->showgrid ) {
	    GDrawDrawLine(pixmap,x+mv->perchar[i].dwidth+mv->perchar[i].kernafter,
		    mv->topend,x+mv->perchar[i].dwidth+mv->perchar[i].kernafter,mv->displayend,0x808080);
	}
	if ( mv->right_to_left )
	    x += mv->perchar[i].kernafter;
	bdfc = mv->bdf==NULL ?	mv->perchar[i].show :
				mv->bdf->chars[mv->perchar[i].sc->enc];
	if ( bdfc==NULL )
    continue;
	x += bdfc->xmin;
	if ( mv->perchar[i].selected )
	    x += mv->activeoff;
	y = ybase - bdfc->ymax;
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
		int scale = 3000/mv->pixelsize, l;
		Color fg, bg;
		if ( scale>4 ) scale = 4; else if ( scale==3 ) scale= 2;
		base.image_type = it_index;
		clut.clut_len = 1<<scale;
		bg = GDrawGetDefaultBackground(NULL);
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
	x = mv->perchar[si].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->width - x;
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

    if ( mv->right_to_left ) {
	/* right to left clipping is hard to think about, it doesn't happen */
	/*  often enough (I think) for me to put the effort to make it efficient */
	GDrawRequestExpose(mv->gw,NULL,false);
return;
    }
    if ( mv->perchar[i].selected )
	off = mv->activeoff;
    r.y = mv->topend; r.height = mv->displayend-mv->topend;
    r.x = mv->perchar[i].dx-mv->xoff; r.width = mv->perchar[i].dwidth+mv->perchar[i].kernafter;
    bdfc = mv->bdf==NULL ? mv->perchar[i].show :
			   mv->bdf->chars[mv->perchar[i].sc->enc];
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
	r.x = mv->width - r.x - r.width;
    GDrawRequestExpose(mv->gw,&r,false);
}

static void MVDeselectChar(MetricsView *mv, int i) {

    mv->perchar[i].selected = false;
    MVRedrawI(mv,i,0,0);
#if 0
    if ( mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[i].dx;
	if ( mv->right_to_left )
	    x = mv->width - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
	GDrawDrawLine(mv->gw,x,mv->topend,x,mv->displayend,0x808080);
	x += mv->perchar[i].dwidth+mv->perchar[i].kernafter;
	GDrawDrawLine(mv->gw,x,mv->topend,x,mv->displayend,0x808080);
    }
#endif
}

static void MVSelectChar(MetricsView *mv, int i) {

    mv->perchar[i].selected = true;
    MVRedrawI(mv,i,0,0);
#if 0
    if ( mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[i].dx;
	if ( x==0 )		/* Not set properly yet */
return;
	if ( mv->right_to_left )
	    x = mv->width - x;
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
    unichar_t ubuf[40];
    char buf[40];

    if ( mv->perchar[i].kern==NULL )	/* Happens during init, we'll do it later */
return;
    for ( kp=mv->perchar[i-1].sc->kerns; kp!=NULL; kp=kp->next )
	if ( kp->sc == mv->perchar[i].sc )
    break;
    sprintf(buf,"%d",kp==NULL?0:kp->off);
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(mv->perchar[i].kern,ubuf);
    mv->perchar[i-1].kernafter = (kp==NULL?0:kp->off) * mv->pixelsize/
	    (mv->fv->sf->ascent+mv->fv->sf->descent);
}

static void MVRefreshValues(MetricsView *mv, int i, SplineChar *sc) {
    unichar_t ubuf[40];
    char buf[40];
    DBounds bb;

    SplineCharFindBounds(sc,&bb);

    uc_strcpy(ubuf,sc->name);
    GGadgetSetTitle(mv->perchar[i].name,ubuf);

    sprintf(buf,"%d",sc->width);
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(mv->perchar[i].width,ubuf);

    sprintf(buf,"%.2f",bb.minx);
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

    sprintf(buf,"%.2f",sc->width-bb.maxx);
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
    GDrawRequestExpose(mv->gw,NULL,false);
}

static BDFChar *MVRasterize(MetricsView *mv,SplineChar *sc) {
    BDFChar *bdfc;

    if ( mv->antialias && mv->pixelsize<1000 ) {
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

    if ( mv->bdf==NULL ) {
	for ( i=0; i<mv->charcnt; ++i ) {
	    BDFChar *bc = MVRasterize(mv,mv->perchar[i].sc);
	    BDFCharFree(mv->perchar[i].show);
	    mv->perchar[i].show = bc;
	    mv->perchar[i].dwidth = bc->width;
	    if ( i+1<mv->charcnt )
		mv->perchar[i+1].dx = mv->perchar[i].dx + bc->width + mv->perchar[i].kernafter;
	}
    }
}

void MVRegenChar(MetricsView *mv, SplineChar *sc) {
    int i;
    BDFChar *bdfc;
    int xoff = 0;
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
		if ( xoff==0 )
		    MVRedrawI(mv,i,oldxmin,oldxmax);
	    }
	    MVRefreshValues(mv,i,sc);
	}
    }
    if ( xoff!=0 )
	GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVChangeDisplayFont(MetricsView *mv, BDFFont *bdf) {
    int i;
    BDFChar *bc;

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
	bc = bdf==NULL?mv->perchar[i].show:bdf->chars[mv->perchar[i].sc->enc];
	mv->perchar[i].dwidth = bc->width;
	if ( i+1<mv->charcnt )
	    mv->perchar[i+1].dx = mv->perchar[i].dx+bc->width+mv->perchar[i].kernafter;
    }
}

static int MV_WidthChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (int) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->perchar[which].sc;
	if ( *end )
	    GDrawBeep(NULL);
	else if ( val!=sc->width ) {
	    SCPreserveWidth(sc);
	    sc->width = val;
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
    int which = (int) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->perchar[which].sc;
	DBounds bb;
	SplineCharFindBounds(sc,&bb);
	if ( *end )
	    GDrawBeep(NULL);
	else if ( val!=bb.minx ) {
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[5] = 0;
	    transform[4] = val-bb.minx;
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
    int which = (int) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->perchar[which].sc;
	DBounds bb;
	SplineCharFindBounds(sc,&bb);
	if ( *end )
	    GDrawBeep(NULL);
	else if ( val!=sc->width-bb.maxx ) {
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

static int MV_KernChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (int) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->perchar[which].sc;
	SplineChar *psc = mv->perchar[which-1].sc;
	static unichar_t zerostr[] = { '0',  '\0' };
	KernPair *kp;
	if ( *end )
	    GDrawBeep(NULL);
	else {
	    for ( kp = psc->kerns; kp!=NULL && kp->sc!=sc; kp = kp->next );
	    if ( kp==NULL ) {
		kp = galloc(sizeof(KernPair));
		kp->sc = sc;
		kp->off = val;
		kp->next = psc->kerns;
		psc->kerns = kp;
	    } else
		kp->off = val;
	    mv->perchar[which-1].kernafter = (val*mv->pixelsize)/
		    (mv->fv->sf->ascent+mv->fv->sf->descent);
	    for ( i=which; i<mv->charcnt; ++i )
		mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth + mv->perchar[i-1].kernafter;
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

    mv->mwidth = 60;
    mv->displayend = mv->height- mv->sbh - 5*(mv->fh+4);
    mv->pixelsize = mv->displayend - mv->topend - 4;

    label.text = (unichar_t *) "Name:";
    label.text_is_1byte = true;
    label.font = mv->font;
    gd.pos.x = 2; gd.pos.width = mv->mwidth-4;
    gd.pos.y = mv->displayend+2;
    gd.pos.height = mv->fh;
    gd.label = &label;
    gd.box = &small;
    gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_dontcopybox;
    mv->namelab = GLabelCreate(mv->gw,&gd,NULL);

    label.text = (unichar_t *) "Width:";
    gd.pos.y += mv->fh+4;
    mv->widthlab = GLabelCreate(mv->gw,&gd,NULL);

    label.text = (unichar_t *) "LBearing:";
    gd.pos.y += mv->fh+4;
    mv->lbearinglab = GLabelCreate(mv->gw,&gd,NULL);

    label.text = (unichar_t *) "RBearing:";
    gd.pos.y += mv->fh+4;
    mv->rbearinglab = GLabelCreate(mv->gw,&gd,NULL);

    label.text = (unichar_t *) "Kern:";
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
    mv->perchar[i].name = GLabelCreate(mv->gw,&gd,(void *) i);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_WidthChanged;
    mv->perchar[i].width = GTextFieldCreate(mv->gw,&gd,(void *) i);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_LBearingChanged;
    mv->perchar[i].lbearing = GTextFieldCreate(mv->gw,&gd,(void *) i);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_RBearingChanged;
    mv->perchar[i].rbearing = GTextFieldCreate(mv->gw,&gd,(void *) i);

    if ( i!=0 ) {
	gd.pos.y += mv->fh+4;
	gd.pos.x -= mv->mwidth/2;
	gd.handle_controlevent = MV_KernChanged;
	mv->perchar[i].kern = GTextFieldCreate(mv->gw,&gd,(void *) i);

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
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }
    mv->perchar[i].sc = sc;
    if ( mv->bdf==NULL ) {
	bdfc = MVRasterize(mv,sc);
	BDFCharFree(mv->perchar[i].show);
	mv->perchar[i].show = bdfc;
    } else
	bdfc = mv->bdf->chars[mv->perchar[i].sc->enc];
    mv->perchar[i].dwidth = bdfc->width;
    if ( mv->perchar[i].width==NULL ) {
	mv->perchar[i].dx = 0;		/* Flag => Don't draw when focus changes (focus changes as we create text fields) */
	MVCreateFields(mv,i);
    }
    if ( i>=mv->charcnt ) mv->charcnt = i+1;
    MVRefreshValues(mv,i,sc);
    if ( i==0 ) {
	mv->perchar[i].dx = 10;
	j=i+1;
    } else {
	mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth + mv->perchar[i-1].kernafter;
	j = i;
    }
    for ( ; j<mv->charcnt; ++j )
	mv->perchar[j].dx = mv->perchar[j-1].dx + mv->perchar[j-1].dwidth + mv->perchar[j-1].kernafter;
}

static SplineChar *SCFromUnicode(SplineFont *sf, int ch) {
    int i;

    if ( sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4 ) {
	if ( ch>=sf->charcnt )
return( NULL );
	else
return( SFMakeChar(sf,ch) );
    } else if ( sf->encoding_name>=em_unicodeplanes && sf->encoding_name<=em_unicodeplanesmax ) {
	ch -= ((sf->encoding_name-em_unicodeplanes)<<16);
	if ( ch>=sf->charcnt || ch<0 )
return( NULL );
	else
return( SFMakeChar(sf,ch) );
    }
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	if ( sf->chars[i]->unicodeenc == ch )
return( sf->chars[i] );

return( NULL );
}

static int SFContainsChar(SplineFont *sf, int ch) {
    int i;

    if ( sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4 )
return( ch<sf->charcnt );
    else if ( sf->encoding_name>=em_unicodeplanes && sf->encoding_name<=em_unicodeplanesmax ) {
	ch -= ((sf->encoding_name-em_unicodeplanes)<<16);
return( ch<sf->charcnt && ch>=0 );
    }
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	if ( sf->chars[i]->unicodeenc == ch )
return( true );

return( false );
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
	if ( wid>mv->width )
return( i-mv->coff );
    }
return( i-mv->coff );		/* There's extra room. don't know exactly how much but allow for some */
}

static void MVSetSb(MetricsView *mv) {
    int cnt = (mv->width-mv->mbase-mv->mwidth)/mv->mwidth;
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

static void MVScroll(MetricsView *mv,struct sbevent *sb) {
    int newpos = mv->coff;
    int cnt = (mv->width-mv->mbase-mv->mwidth)/mv->mwidth;
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
	charrect.x = 0; charrect.width = mv->width;
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
	GDrawIError("No change when there should have been one in MV_TextChanged");
    if ( u_strlen(ret)>=mv->max ) {
	int oldmax=mv->max;
	mv->max = u_strlen(ret)+10;
	mv->perchar = grealloc(mv->perchar,mv->max*sizeof(struct metricchar));
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }

    missing = 0;
    for ( tpt=pt; tpt<ept; ++tpt )
	if ( !SFContainsChar(mv->fv->sf,*tpt))
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
	SplineChar *sc = SCFromUnicode(mv->fv->sf,*pt);
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
    r.width = mv->width;
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


#define MID_Next	2005
#define MID_Prev	2006
#define MID_Outline	2007
#define MID_ShowGrid	2008
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_AntiAlias	2014
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
#define MID_CleanupChar	2225
#define MID_Center	2600
#define MID_OpenBitmap	2700
#define MID_OpenOutline	2701
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
#define MID_Center	2600
#define MID_Thirds	2604
#define MID_Recent	2703

static void MVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GDrawDestroyWindow(gw);
}

static void MVMenuOpenBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( mv->fv->sf->bitmaps==NULL )
return;
    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->charcnt )
	BitmapViewCreatePick(mv->perchar[i].sc->enc,mv->fv);
}

static void MVMenuMergeKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MergeKernInfo(mv->fv->sf);
}

static void MVMenuOpenOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->charcnt )
	CharViewCreate(mv->perchar[i].sc,mv->fv);
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
    _FVMenuGenerate(mv->fv);
}

static void MVMenuPrint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    PrintDlg(NULL,NULL,mv);
}

static void MVUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_undo) )
	MVTextChanged(mv);
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	if ( mv->perchar[i].sc->undoes[dm_fore]!=NULL )
	    SCDoUndo(mv->perchar[i].sc,dm_fore);
    }
}

static void MVRedo(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_redo) )
	MVTextChanged(mv);
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	if ( mv->perchar[i].sc->redoes[dm_fore]!=NULL )
	    SCDoRedo(mv->perchar[i].sc,dm_fore);
    }
}

static void MVClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    static int buts[] = { _STR_Yes, _STR_Unlink, _STR_Cancel, 0 };
    BDFFont *bdf;
    extern int onlycopydisplayed;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_clear) )
	MVTextChanged(mv);
    else {
	for ( i=mv->charcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	sc = mv->perchar[i].sc;
	if ( sc->dependents!=NULL ) {
	    int yes = GWidgetAskCenteredR(_STR_BadReference,buts,1,2,_STR_ClearDependent,sc->name);
	    if ( yes==2 )
return;
	    if ( yes==1 )
		UnlinkThisReference(NULL,sc);
	}

	if ( onlycopydisplayed && mv->bdf==NULL ) {
	    SCClearAll(sc);
	} else if ( onlycopydisplayed ) {
	    BCClearAll(mv->bdf->chars[i]);
	} else {
	    SCClearAll(sc);
	    for ( bdf=mv->fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next )
		BCClearAll(bdf->chars[i]);
	}
    }
}

static void MVCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_cut) )
	MVTextChanged(mv);
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
	MVTextChanged(mv);
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

static void MVPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_paste) )
	MVTextChanged(mv);
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
    for ( rf=sc->refs; rf!=NULL ; rf=next ) {
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
	SCGetInfo(mv->perchar[i].sc,true);
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
	int dobackground ) {
    SplineChar *sc = _sc;

    FVTrans(sc->parent->fv,sc,transform, NULL,dobackground);
}

static void MVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	TransformDlgCreate( mv->perchar[i].sc,MVTransFunc,getorigin,true );
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

static void MVMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	MinimumDistancesFree(sc->md);
	sc->md = NULL;
	sc->splines = SplineSetRemoveOverlap(sc->splines);
	SCCharChangedUpdate(sc);
    }
}

static void MVSimplify( MetricsView *mv,int type ) {
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	sc->splines = SplineCharSimplify(sc,sc->splines,type);
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
	SplineCharAddExtrema(sc->splines,false);
	SCCharChangedUpdate(sc);
    }
}

static void MVMenuRound2Int(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCRound2Int( mv->perchar[i].sc, mv->fv);
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
	SCPreserveState(sc,false);
	sc->splines = SplineSetsCorrect(sc->splines);
	SCCharChangedUpdate(sc);
    }
}

static void MVMenuBuildAccent(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    int onlyaccents = e==NULL || !(e->u.mouse.state&ksm_shift);
    extern int onlycopydisplayed;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	if ( SFIsRotatable(mv->fv->sf,sc) ||
		(SCMakeDotless(mv->fv->sf,sc,false,false) && !onlyaccents) ||
		(SFIsCompositBuildable(mv->fv->sf,sc->unicodeenc,sc) &&
		 (!onlyaccents || hascomposing(mv->fv->sf,sc->unicodeenc,sc)))) {
	    SCBuildComposit(mv->fv->sf,sc,!onlycopydisplayed,mv->fv);
	}
    }
}

static void MVResetText(MetricsView *mv) {
    unichar_t *new, *pt;
    int i,si;

    new = galloc((mv->charcnt+1)*sizeof(unichar_t));
    si=-1;
    for ( pt=new, i=0; i<mv->charcnt; ++i ) {
	if ( mv->perchar[i].sc->unicodeenc==-1 )
	    *pt++ = 0xfffd;
	else
	    *pt++ = mv->perchar[i].sc->unicodeenc;
	if ( mv->perchar[i].selected ) si=i;
    }
    *pt = '\0';
    GGadgetSetTitle(mv->text,new);
    free(new );
}

static void MVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->fv->sf;
    int i, pos;

    for ( i=0; i<mv->charcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->charcnt ) {
	if ( mi->mid == MID_Next ) {
	    pos = mv->perchar[i].sc->enc+1;
	} else if ( mi->mid==MID_Prev ) {
	    pos = mv->perchar[i].sc->enc-1;
	} else if ( mi->mid==MID_NextDef ) {
	    for ( pos = mv->perchar[i].sc->enc+1; pos<sf->charcnt && sf->chars[pos]==NULL; ++pos );
	    if ( pos>=sf->charcnt )
return;
	} else if ( mi->mid==MID_PrevDef ) {
	    for ( pos = mv->perchar[i].sc->enc-1; pos>=0 && sf->chars[pos]==NULL; --pos );
	    if ( pos<0 )
return;
	}
	if ( pos>=0 && pos<mv->fv->sf->charcnt ) {
	    MVSetPos(mv,i,SFMakeChar(mv->fv->sf,pos));
	    /* May need to adjust start of current char if kerning changed */
	    for ( /*i++*/ ; i<mv->charcnt; ++i )
		mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth + mv->perchar[i-1].kernafter;
	    GDrawRequestExpose(mv->gw,NULL,false);
	    MVResetText(mv);
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
	    FVTrans(mv->fv,sc,transform,NULL,false);
    }
}

static GMenuItem dummyitem[] = { { (unichar_t *) _STR_New, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) _STR_New, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, 'N', ksm_control, NULL, NULL, MenuNew },
    { { (unichar_t *) _STR_Open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) _STR_Close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, MVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Openoutline, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'u' }, 'H', ksm_control, NULL, NULL, MVMenuOpenOutline },
    { { (unichar_t *) _STR_Openbitmap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'B' }, 'J', ksm_control, NULL, NULL, MVMenuOpenBitmap, MID_OpenBitmap },
    { { (unichar_t *) _STR_Openmetrics, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'M' }, 'K', ksm_control, NULL, NULL, /* No function, never avail */NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, MVMenuSave },
    { { (unichar_t *) _STR_Saveas, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, MVMenuSaveAs },
    { { (unichar_t *) _STR_Generate, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'G' }, 'G', ksm_control|ksm_shift, NULL, NULL, MVMenuGenerate },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Mergekern, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'M' }, 'K', ksm_control|ksm_shift, NULL, NULL, MVMenuMergeKern },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Print, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'P', ksm_control, NULL, NULL, MVMenuPrint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) _STR_Undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL, MVUndo, MID_Undo },
    { { (unichar_t *) _STR_Redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL, MVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, MVCut, MID_Cut },
    { { (unichar_t *) _STR_Copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, MVCopy, MID_Copy },
    { { (unichar_t *) _STR_Copyref, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'o' }, 'G', ksm_control, NULL, NULL, MVMenuCopyRef, MID_CopyRef },
    { { (unichar_t *) _STR_Copywidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 'W', ksm_control, NULL, NULL, MVMenuCopyWidth, MID_CopyWidth },
    { { (unichar_t *) _STR_CopyVWidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'V' }, '\0', ksm_control, NULL, NULL, MVMenuCopyWidth, MID_CopyVWidth },
    { { (unichar_t *) _STR_CopyLBearing, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'p' }, '\0', ksm_control, NULL, NULL, MVMenuCopyWidth, MID_CopyLBearing },
    { { (unichar_t *) _STR_CopyRBearing, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'g' }, '\0', ksm_control, NULL, NULL, MVMenuCopyWidth, MID_CopyRBearing },
    { { (unichar_t *) _STR_Paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, MVPaste, MID_Paste },
    { { (unichar_t *) _STR_Clear, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'l' }, 0, 0, NULL, NULL, MVClear, MID_Clear },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_SelectAll, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, MVSelectAll, MID_SelAll },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Unlinkref, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'U', ksm_control, NULL, NULL, MVUnlinkRef, MID_UnlinkRef },
    { NULL }
};

static GMenuItem ellist[] = {
    { { (unichar_t *) _STR_Fontinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 'F', ksm_control|ksm_shift, NULL, NULL, MVMenuFontInfo },
    { { (unichar_t *) _STR_Charinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'I' }, 'I', ksm_control, NULL, NULL, MVMenuCharInfo, MID_CharInfo },
    { { (unichar_t *) _STR_ShowDependents, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'D' }, 'I', ksm_control|ksm_meta, NULL, NULL, MVMenuShowDependents, MID_ShowDependents },
    { { (unichar_t *) _STR_Findprobs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'o' }, 'E', ksm_control, NULL, NULL, MVMenuFindProblems, MID_FindProblems },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Bitmapsavail, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'B', ksm_control|ksm_shift, NULL, NULL, MVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) _STR_Regenbitmaps, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'B' }, 'B', ksm_control, NULL, NULL, MVMenuBitmaps, MID_RegenBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Transform, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'T' }, '\\', ksm_control, NULL, NULL, MVMenuTransform, MID_Transform },
    { { (unichar_t *) _STR_Stroke, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 'E', ksm_control|ksm_shift, NULL, NULL, MVMenuStroke, MID_Stroke },
    { { (unichar_t *) _STR_Rmoverlap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control|ksm_shift, NULL, NULL, MVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) _STR_Simplify, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'M', ksm_control|ksm_shift, NULL, NULL, MVMenuSimplify, MID_Simplify },
    { { (unichar_t *) _STR_CleanupChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, MVMenuCleanup, MID_CleanupChar },
    { { (unichar_t *) _STR_AddExtrema, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'x' }, 'X', ksm_control|ksm_shift, NULL, NULL, MVMenuAddExtrema, MID_AddExtrema },
    { { (unichar_t *) _STR_Round2int, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'I' }, '_', ksm_control|ksm_shift, NULL, NULL, MVMenuRound2Int, MID_Round },
    { { (unichar_t *) _STR_MetaFont, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'M' }, '!', ksm_control|ksm_shift, NULL, NULL, MVMenuMetaFont, MID_MetaFont },
    { { (unichar_t *) _STR_Autotrace, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'r' }, 'T', ksm_control|ksm_shift, NULL, NULL, MVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Correct, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'D' }, 'D', ksm_control|ksm_shift, NULL, NULL, MVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Buildaccent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'B' }, 'A', ksm_control|ksm_shift, NULL, NULL, MVMenuBuildAccent, MID_BuildAccent },
    { NULL }
};

static GMenuItem vwlist[] = {
    { { (unichar_t *) _STR_NextChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, ']', ksm_control, NULL, NULL, MVMenuChangeChar, MID_Next },
    { { (unichar_t *) _STR_PrevChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, '[', ksm_control, NULL, NULL, MVMenuChangeChar, MID_Prev },
    { { (unichar_t *) _STR_NextDefChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'D' }, ']', ksm_control|ksm_meta, NULL, NULL, MVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) _STR_PrevDefChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, '[', ksm_control|ksm_meta, NULL, NULL, MVMenuChangeChar, MID_PrevDef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Hidegrid, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'G' }, '\0', ksm_control, NULL, NULL, MVMenuShowGrid, MID_ShowGrid },
    { { (unichar_t *) _STR_Antialias, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'A' }, '5', ksm_control, NULL, NULL, MVMenuAA, MID_AntiAlias },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Outline, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'O' }, '\0', ksm_control, NULL, NULL, MVMenuShowBitmap, MID_Outline },
    { NULL },			/* Some extra room to show bitmaps */
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }
};

static GMenuItem mtlist[] = {
    { { (unichar_t *) _STR_Center, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, MVMenuCenter, MID_Center },
    { { (unichar_t *) _STR_Thirds, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, MVMenuCenter, MID_Thirds },
    { NULL }
};

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_OpenBitmap:
	    mi->ti.disabled = mv->fv->sf->bitmaps==NULL;
	  break;
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
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
	  case MID_Copy: case MID_CopyRef: case MID_CopyWidth:
	  case MID_CopyLBearing: case MID_CopyRBearing:
	  case MID_Cut: case MID_Clear:
	    mi->ti.disabled = i==-1;
	  break;
	  case MID_CopyVWidth:
	    mi->ti.disabled = i==-1 || !mv->fv->sf->hasvmetrics;
	  break;
	  case MID_Undo:
	    mi->ti.disabled = i==-1 || mv->perchar[i].sc->undoes[dm_fore]==NULL;
	  break;
	  case MID_Redo:
	    mi->ti.disabled = i==-1 || mv->perchar[i].sc->redoes[dm_fore]==NULL;
	  break;
	  case MID_UnlinkRef:
	    mi->ti.disabled = i==-1 || mv->perchar[i].sc->refs==NULL;
	  break;
	  case MID_Paste:
	    mi->ti.disabled = i==-1 || !CopyContainsSomething();
	  break;
	}
    }
}

static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, anybuildable, onlyaccents;
    SplineChar *sc;

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
	    mi->ti.disabled = sc==NULL || mv->fv->cidmaster!=NULL;
	  break;
	  case MID_ShowDependents:
	    mi->ti.disabled = sc==NULL || sc->dependents == NULL;
	  break;
	  case MID_FindProblems:
	  case MID_MetaFont:
	  case MID_Transform:
	    mi->ti.disabled = sc==NULL;
	  break;
	  case MID_AddExtrema:
	  case MID_Stroke: case MID_RmOverlap:
	  case MID_Round: case MID_Correct:
	    mi->ti.disabled = sc==NULL || mv->fv->sf->onlybitmaps;
	  break;
	  case MID_Simplify:
	    mi->ti.disabled = sc==NULL || mv->fv->sf->onlybitmaps;
	    free(mi->ti.text);
	    if ( e==NULL || !(e->u.mouse.state&ksm_shift) ) {
		mi->ti.text = u_copy(GStringGetResource(_STR_Simplify,NULL));
		mi->short_mask = ksm_control|ksm_shift;
		mi->invoke = MVMenuSimplify;
	    } else {
		mi->ti.text = u_copy(GStringGetResource(_STR_SimplifyMore,NULL));
		mi->short_mask = (ksm_control|ksm_meta|ksm_shift);
		mi->invoke = MVMenuSimplifyMore;
	    }
	  break;
	  case MID_BuildAccent:
	    anybuildable = false;
	    onlyaccents = e==NULL || !(e->u.mouse.state&ksm_shift);
	    if ( sc!=NULL && ( SFIsRotatable(mv->fv->sf,sc) ||
			(SCMakeDotless(mv->fv->sf,sc,false,false) && !onlyaccents) ||
			(SFIsCompositBuildable(mv->fv->sf,sc->unicodeenc,sc) &&
			 (!onlyaccents || hascomposing(mv->fv->sf,sc->unicodeenc,sc))) ) ) {
		anybuildable = true;
	    }
	    mi->ti.disabled = !anybuildable;
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(onlyaccents?_STR_Buildaccent:_STR_Buildcomposit,NULL));
	  break;
	  case MID_Autotrace:
	    mi->ti.disabled = !(FindAutoTraceName()!=NULL && sc!=NULL &&
		    sc->backimages!=NULL );
	  break;
	}
    }
}

static void vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, base;
    BDFFont *bdf;
    char buffer[50];
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt);

    for ( i=0; vwlist[i].mid!=MID_Outline; ++i )
	if ( vwlist[i].mid==MID_ShowGrid ) {
	    vwlist[i].ti.text = (unichar_t *) GStringGetResource(mv->showgrid?_STR_Hidegrid:_STR_Showgrid,NULL);
	    vwlist[i].ti.text_in_resource = false;
	} else if ( vwlist[i].mid==MID_AntiAlias ) {
	    vwlist[i].ti.checked = mv->antialias;
	    vwlist[i].ti.disabled = mv->bdf!=NULL;
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
	    sprintf( buffer, "%d pixel bitmap", bdf->pixelsize );
	    vwlist[i].ti.text = uc_copy(buffer);
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

static GMenuItem mblist[] = {
    { { (unichar_t *) _STR_File, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) _STR_Edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 0, 0, edlist, edlistcheck },
    { { (unichar_t *) _STR_Element, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'l' }, 0, 0, ellist, ellistcheck },
    { { (unichar_t *) _STR_View, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { (unichar_t *) _STR_Metric, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'M' }, 0, 0, mtlist, NULL },
    { { (unichar_t *) _STR_Window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 0, 0, NULL, WindowMenuBuild, NULL },
    { { (unichar_t *) _STR_Help, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};

static void MVResize(MetricsView *mv,GEvent *event) {
    GRect pos;
    int i;

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

    pos.width = event->u.resize.size.width;
    pos.height = mv->sbh;
    pos.y = event->u.resize.size.height - pos.height; pos.x = 0;
    GGadgetResize(mv->hsb,pos.width,pos.height);
    GGadgetMove(mv->hsb,pos.x,pos.y);

    mv->width = event->u.resize.size.width;
    mv->displayend = event->u.resize.size.height - (mv->height-mv->displayend);
    mv->height = event->u.resize.size.height;

    mv->pixelsize = mv->displayend - mv->topend - 4;

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
    GGadgetMove(mv->namelab,2,mv->displayend+2);
    GGadgetMove(mv->widthlab,2,mv->displayend+2+mv->fh+4);
    GGadgetMove(mv->lbearinglab,2,mv->displayend+2+2*(mv->fh+4));
    GGadgetMove(mv->rbearinglab,2,mv->displayend+2+3*(mv->fh+4));
    GGadgetMove(mv->kernlab,2,mv->displayend+2+4*(mv->fh+4));

    MVReRasterize(mv);

    GDrawRequestExpose(mv->gw,NULL,true);
}

static void MVChar(MetricsView *mv,GEvent *event) {
    if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuSaveAll(NULL,NULL,NULL);
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

static void MVMouse(MetricsView *mv,GEvent *event) {
    int i, x, j, within, sel, ybase;
    SplineChar *sc;
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
		SCPreparePopup(mv->gw,mv->perchar[i].sc);
	}
	if ( mv->cursor!=ct_mypointer ) {
	    GDrawSetCursor(mv->gw,ct_mypointer);
	    mv->cursor = ct_mypointer;
	}
return;
    }

    event->u.mouse.x += mv->xoff;
    ybase = mv->topend + 2 + (mv->pixelsize * mv->fv->sf->ascent /
	    (mv->fv->sf->ascent+mv->fv->sf->descent));
    within = -1;
    for ( i=0; i<mv->charcnt; ++i ) {
	x = mv->perchar[i].dx;
	if ( mv->right_to_left )
	    x = mv->width - x - mv->perchar[i].dwidth - + mv->perchar[i].kernafter;
	if ( mv->bdf==NULL )
	    if ( event->u.mouse.x >= x+mv->perchar[i].show->xmin &&
		event->u.mouse.x <= x+mv->perchar[i].show->xmax &&
		event->u.mouse.y <= ybase-mv->perchar[i].show->ymin &&
		event->u.mouse.y >= ybase-mv->perchar[i].show->ymax &&
		hitsbit(mv->perchar[i].show,event->u.mouse.x-x-mv->perchar[i].show->xmin,
			mv->perchar[i].show->ymax-(ybase-event->u.mouse.y)) )
    break;
	if ( event->u.mouse.x >= x && event->u.mouse.x < x+mv->perchar[i].dwidth+ mv->perchar[i].kernafter )
	    within = i;
    }
    if ( i==mv->charcnt )
	sc = NULL;
    else
	sc = mv->perchar[i].sc;

    diff = event->u.mouse.x-mv->pressed_x;
    /*if ( mv->right_to_left ) diff = -diff;*/
    sel = onwidth = onkern = false;
    if ( sc==NULL ) {
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
		    event->u.mouse.x>mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter-3 ) {
		onwidth = true;
		sel = true;
	    }
	} else {
	    if ( within>0 && mv->perchar[within-1].selected &&
		    event->u.mouse.x>mv->width-(mv->perchar[within].dx+3) )
		onwidth = true;		/* previous char */
	    else if ( within!=-1 && within+1<mv->charcnt &&
		    mv->perchar[within+1].selected &&
		    event->u.mouse.x<mv->width-(mv->perchar[within+1].dx-3) )
		onkern = true;			/* subsequent char */
	    else if ( within>=0 && mv->perchar[within].selected &&
		    event->u.mouse.x>mv->width-(mv->perchar[within].dx+3) )
		onkern = true;
	    else if ( within>=0 &&
		    event->u.mouse.x<mv->width-(mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter-3) ) {
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
	    SCPreparePopup(mv->gw,sc);
/* Don't allow any editing when displaying a bitmap font */
    } else if ( event->type == et_mousedown && mv->bdf==NULL ) {
	CVPaletteDeactivate();
	if ( sc!=NULL ) {
	    for ( j=0; j<mv->charcnt; ++j )
		if ( j!=i && mv->perchar[j].selected )
		    MVDeselectChar(mv,j);
	    MVSelectChar(mv,i);
	    GWindowClearFocusGadgetOfWindow(mv->gw);
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
	} else if ( !mv->right_to_left &&
		event->u.mouse.x<mv->perchar[mv->charcnt-1].dx+mv->perchar[mv->charcnt-1].dwidth+mv->perchar[mv->charcnt-1].kernafter+3 ) {
	    mv->pressed = mv->pressedwidth = true;
	    GDrawSetCursor(mv->gw,ct_rbearing);
	    mv->cursor = ct_rbearing;
	    if ( !mv->perchar[mv->charcnt-1].selected )
		    MVDoSelect(mv,mv->charcnt-1);
	} else if ( mv->right_to_left &&
		event->u.mouse.x>mv->width - (mv->perchar[mv->charcnt-1].dx+mv->perchar[mv->charcnt-1].dwidth+mv->perchar[mv->charcnt-1].kernafter+3) ) {
	    mv->pressed = mv->pressedwidth = true;
	    GDrawSetCursor(mv->gw,ct_rbearing);
	    mv->cursor = ct_rbearing;
	    if ( !mv->perchar[mv->charcnt-1].selected )
		    MVDoSelect(mv,mv->charcnt-1);
	}
	mv->pressed_x = event->u.mouse.x;
    } else if ( event->type == et_mousemove && mv->pressed ) {
	for ( i=0; i<mv->charcnt && !mv->perchar[i].selected; ++i );
	if ( mv->pressedwidth ) {
	    int ow = mv->perchar[i].dwidth;
	    if ( mv->right_to_left ) diff = -diff;
	    mv->perchar[i].dwidth = mv->perchar[i].show->width + diff;
	    if ( ow!=mv->perchar[i].dwidth ) {
		for ( j=i+1; j<mv->charcnt; ++j )
		    mv->perchar[j].dx = mv->perchar[j-1].dx+mv->perchar[j-1].dwidth+ mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    int ow = mv->perchar[i-1].kernafter;
	    KernPair *kp;
	    int kpoff;
	    for ( kp = mv->perchar[i-1].sc->kerns; kp!=NULL && kp->sc!=mv->perchar[i].sc; kp = kp->next );
	    kpoff = (kp==NULL?0:kp->off) * mv->pixelsize /
		    (mv->fv->sf->descent+mv->fv->sf->ascent);
	    if ( mv->right_to_left ) diff = -diff;
	    mv->perchar[i-1].kernafter = kpoff + diff;
	    if ( ow!=mv->perchar[i-1].kernafter ) {
		for ( j=i; j<mv->charcnt; ++j )
		    mv->perchar[j].dx = mv->perchar[j-1].dx+mv->perchar[j-1].dwidth+ mv->perchar[j-1].kernafter;
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
	    CharViewCreate(mv->perchar[within].sc,mv->fv);
	else
	    BitmapViewCreate(mv->bdf->chars[mv->perchar[within].sc->enc],mv->bdf,mv->fv);
    } else if ( event->type == et_mouseup && mv->pressed ) {
	for ( i=0; i<mv->charcnt && !mv->perchar[i].selected; ++i );
	mv->pressed = false;
	mv->activeoff = 0;
	sc = mv->perchar[i].sc;
	if ( mv->pressedwidth ) {
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
		for ( kp = mv->perchar[i-1].sc->kerns; kp!=NULL && kp->sc!=mv->perchar[i].sc; kp = kp->next );
		if ( kp==NULL ) {
		    kp = gcalloc(1,sizeof(KernPair));
		    kp->sc = mv->perchar[i].sc;
		    kp->next = mv->perchar[i-1].sc->kerns;
		    mv->perchar[i-1].sc->kerns = kp;
		}
		if ( mv->right_to_left ) diff = -diff;
		kp->off += diff;
		mv->perchar[i-1].kernafter = kp->off*mv->pixelsize/
			(mv->fv->sf->ascent+mv->fv->sf->descent);
		MVRefreshValues(mv,i-1,mv->perchar[i-1].sc);
		for ( ; i<mv->charcnt; ++i )
		    mv->perchar[i].dx = mv->perchar[i-1].dx+mv->perchar[i-1].dwidth +
			    mv->perchar[i-1].kernafter;
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
	for ( j=0; j<mv->charcnt; ++j )
	    if ( j!=within && mv->perchar[j].selected )
		MVDeselectChar(mv,j);
	MVSelectChar(mv,within);
    }
}

static int mv_e_h(GWindow gw, GEvent *event) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
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
	MVMouse(mv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    MVScroll(mv,&event->u.control.u.sb);
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
    char buffer[100];
    unichar_t ubuf[100];
    GTextInfo label;
    int i,cnt;
    int as,ds,ld;

    if ( icon==NULL )
	icon = GDrawCreateBitmap(NULL,metricsicon_width,metricsicon_height,metricsicon_bits);

    mv->fv = fv;
    mv->bdf = bdf;
    mv->showgrid = true;
    mv->antialias = mv_antialias;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_mypointer;
    sprintf(buffer, "Metrics for %.80s", fv->sf->fontname);
    uc_strcpy(ubuf,buffer);
    wattrs.window_title = ubuf;
    wattrs.icon = icon;
    pos.x = pos.y = 0;
    pos.width = 800;
    pos.height = 300;
    mv->gw = gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,mv,&wattrs);
    mv->width = pos.width; mv->height = pos.height;

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
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

    memset(&rq,0,sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    mv->font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(mv->font,&as,&ds,&ld);
    mv->fh = as+ds; mv->as = as;

    mv->perchar = gcalloc(mv->max=20,sizeof(struct metricchar));
    if ( sc!=NULL ) {
	ubuf[0] = sc->unicodeenc==-1 ? 0xfffd: sc->unicodeenc;
	mv->perchar[0].sc = sc;
	cnt = 1;
    } else {
	for ( i=cnt=0; i<fv->sf->charcnt && cnt<15; ++i ) {
	    if ( fv->selected[i] && fv->sf->chars[i]!=NULL ) {
		mv->perchar[cnt].sc = fv->sf->chars[i];
		ubuf[cnt++] = fv->sf->chars[i]->unicodeenc==-1 ? 0xfffd: fv->sf->chars[i]->unicodeenc;
	    }
	}
    }
    if ( cnt!=0 ) {
	MVSelectChar(mv,cnt-1);
	mv->right_to_left = isrighttoleft(ubuf[0])?1:0;
    }
    ubuf[cnt] = 0;
    mv->charcnt = cnt;

    memset(&gd,0,sizeof(gd));
    memset(&label,0,sizeof(label));
    gd.pos.y = mv->mbh+2; gd.pos.x = 10;
    gd.pos.width = GDrawPointsToPixels(mv->gw,200);
    gd.label = &label;
    label.text = ubuf;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gd.handle_controlevent = MV_TextChanged;
    mv->text = GTextFieldCreate(gw,&gd,mv);
    GGadgetGetSize(mv->text,&gsize);
    mv->topend = gsize.y + gsize.height + 2;
    MVMakeLabels(mv);

    if ( sc!=NULL ) {
	MVSetPos(mv,0,sc);
    } else {
	for ( i=cnt=0; i<fv->sf->charcnt && cnt<15; ++i ) {
	    if ( fv->selected[i] && fv->sf->chars[i]!=NULL ) {
		MVSetPos(mv,cnt++,fv->sf->chars[i]);
	    }
	}
    }
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
