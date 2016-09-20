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
#include "autotrace.h"
#include "autowidth.h"
#include "bitmapchar.h"
#include "fontforgeui.h"
#include "cvundoes.h"
#include "lookups.h"
#include <gkeysym.h>
#include <gresource.h>
#include <gresedit.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>
#include <stdio.h>

#include "collabclientui.h"
#include "gfile.h"
#include "wordlistparser.h"
extern char* SFDCreateUndoForLookup( SplineFont *sf, int lookup_type ) ;


int mv_width = 800, mv_height = 300;
int mvshowgrid = mv_hidegrid;
int mv_type = mv_widthonly;
static int mv_antialias = true;
static double mv_scales[] = { 8.0, 4.0, 2.0, 1.5, 1.0, 2.0/3.0, .5, 1.0/3.0, .25, .2, 1.0/6.0, .125, .1 };
#define SCALE_INDEX_NORMAL	4

static Color widthcol = 0x808080;
static Color italicwidthcol = 0x909090;
static Color selglyphcol = 0x909090;
static Color kernlinecol = 0x008000;
static Color rbearinglinecol = 0x000080;

int pref_mv_shift_and_arrow_skip = 10;
int pref_mv_control_shift_and_arrow_skip = 5;

static void MVSelectChar(MetricsView *mv, int i);
static void MVSelectSetForAll(MetricsView *mv, int selected );
static void MVMoveInWordListByOffset( MetricsView *mv, int offset );

static int MVMoveToNextInWordList(GGadget *g, GEvent *e)
{
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
        MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
        MVMoveInWordListByOffset( mv, 1 );
    }
    return 1;
}
static int MVMoveToPrevInWordList(GGadget *g, GEvent *e)
{
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
        MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
        MVMoveInWordListByOffset( mv, -1 );
    }
    return 1;
}

static void MVMoveInTableByColumnByOffset(MetricsView *mv, int offset) {
	int current_pos = 0;
	// Find the currently selected record.
	for (current_pos = 0; current_pos < mv->clen && mv->perchar[current_pos].selected == 0; current_pos ++);
	// Return on failure.
	if (current_pos >= mv->clen || mv->perchar[current_pos].selected == 0) return;
	// Ensure that we can move ahead the selected number of records. Return otherwise.
	if (current_pos + offset >= mv->clen) return;
	// Change the selection.
	mv->perchar[current_pos].selected = 0;
	mv->perchar[current_pos + offset].selected = 1;
	// Find the currently selected gadget.
	GGadget *current_gadget = GWindowGetFocusGadgetOfWindow(mv->gw);
	// Find which control in the current record it is.
	int current_gadget_type = 0; // 0 is nothing, 1 is Name, 2 is Width, 3 is LBearing, 4 is RBearing, 5 is Kern.
	// We do not currently use this value, but it seems likely to be useful.
	GGadget *target_gadget = NULL;
	if (current_gadget == mv->perchar[current_pos].name) {
		current_gadget_type = 1;
		target_gadget = mv->perchar[current_pos+offset].name;
	} else if (current_gadget == mv->perchar[current_pos].width) {
		current_gadget_type = 2;
		target_gadget = mv->perchar[current_pos+offset].width;
	} else if (current_gadget == mv->perchar[current_pos].lbearing) {
		current_gadget_type = 3;
		target_gadget = mv->perchar[current_pos+offset].lbearing;
	} else if (current_gadget == mv->perchar[current_pos].rbearing) {
		current_gadget_type = 4;
		target_gadget = mv->perchar[current_pos+offset].rbearing;
	} else if (current_gadget == mv->perchar[current_pos].kern) {
		current_gadget_type = 5;
		target_gadget = mv->perchar[current_pos+offset].kern;
	}
	// Abort if there is no selected control for the current record.
	if (current_gadget_type == 0) return;
	// Change the control focus.
	if (target_gadget != NULL) {
		GWidgetIndicateFocusGadget(target_gadget);
	}
	return;
}

/**
 * This doesn't need to be a perfect test by any means. It should
 * return true if the currently active kerning lookup includes some
 * class based kerning which might require GUI elements other than the
 * currently active one to be drawn. The only price to pay by
 * returning true all the time is a slight performance one when
 * redrawing something that doesn't absolutely need to be redrawn.
 */
static int haveClassBasedKerningInView( MetricsView* mv )
{
    if( mv->cur_subtable )
    {
	return mv->cur_subtable->kc > 0;
    }
    return 0;
}

static SplineChar* getSelectedChar( MetricsView* mv ) {
    int i=0;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	if ( mv->perchar[i].selected ) {
	    return mv->chars[i];
	}
    }
    if( mv->glyphcnt==1 ) {
	return mv->chars[0];
    }
    return 0;
}


static void selectUserChosenWordListGlyphs( MetricsView *mv, void* userdata )
{
    printf("selectUserChosenWordListGlyphs(top)\n");
    MVSelectSetForAll( mv, 0 );
    // The previous check thought that userdata was in integer and wanted to verify that
    // it was positive and not equal to -1 or to -2. Frank changed it.
    if( userdata != NULL)
    {
	if (userdata == (void*)(-1) || userdata == (void*)(-2))
	  fprintf(stderr, "Possible error; see the code here.\n");

	WordListLine wll = (WordListLine)userdata;
	for( ; wll->sc; wll++ ) {
	    if( wll->isSelected ) {
		MVSelectChar( mv, wll->currentGlyphIndex );
	    }
	}
    }
}

static int MVGetSplineFontPieceMealFlags( MetricsView *mv )
{
    int ret = 0;

    ret = pf_ft_recontext;

    if( mv->antialias )
	ret |= pf_antialias;
    if( !mv->usehinting )
	ret |= pf_ft_nohints;

    return ret;
}


void MVColInit( void ) {
    static int cinit=false;
    GResStruct mvcolors[] = {
	{ "AdvanceWidthColor", rt_color, &widthcol, NULL, 0 },
	{ "ItalicAdvanceColor", rt_color, &italicwidthcol, NULL, 0 },
	{ "SelectedGlyphColor", rt_color, &selglyphcol, NULL, 0 },
	{ "KernLineColor", rt_color, &kernlinecol, NULL, 0 },
	{ "SideBearingLineColor", rt_color, &rbearinglinecol, NULL, 0 },
	GRESSTRUCT_EMPTY
    };
    if ( !cinit ) {
	GResourceFind( mvcolors, "MetricsView.");
	cinit = true;
    }
}

static int MVSetVSb(MetricsView *mv);

static int MVShowGrid(MetricsView *mv) {
    if ( mv->showgrid==mv_hidegrid || (mv->showgrid==mv_hidemovinggrid && mv->pressed ))
return( false );

return( true );
}

static void MVDrawLine(MetricsView *mv,GWindow pixmap,
	int xtop, int top,int xbot,int bot,Color col) {
    if ( mv->showgrid == mv_partialgrid ) {
	int y1, y2;
	int x1, x2;
	y1 =  bot +   ( top- bot)/4;
	x1 = xbot +   (xtop-xbot)/4;
	y2 =  bot + 4*( top- bot)/5;
	x2 = xbot + 4*(xtop-xbot)/5;
	GDrawDrawLine(pixmap,xtop,top,x2,y2,col);
	GDrawDrawLine(pixmap,x1,y1,xbot,bot,col);
    } else
	GDrawDrawLine(pixmap,xtop,top,xbot,bot,col);
}

static void MVSubVExpose(MetricsView *mv, GWindow pixmap, GEvent *event) {
    /* Expose routine for vertical metrics */
    GRect *clip;
    int xbase, y, si, i, x, width, height;
    double iscale = mv->pixelsize_set_by_window ? 1.0 : mv_scales[mv->scale_index];
    int as = rint(iscale*mv->pixelsize*mv->sf->ascent/(double) (mv->sf->ascent+mv->sf->descent));
    BDFChar *bdfc;
    struct _GImage base;
    GImage gi;
    GClut clut;

    clip = &event->u.expose.rect;

    xbase = mv->vwidth/2;
    if ( mv->showgrid )
	GDrawDrawLine(pixmap,xbase,0,xbase,mv->vheight,widthcol);

    if ( mv->bdf==NULL && MVShowGrid(mv) ) {
	y = mv->perchar[0].dy-mv->yoff;
	MVDrawLine(mv,pixmap,0,y,mv->vwidth,y,widthcol);
    }

    si = -1;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	if ( mv->perchar[i].selected ) si = i;
	y = mv->perchar[i].dy-mv->yoff;
	if ( mv->bdf==NULL &&  MVShowGrid(mv)) {
	    int yp = y+mv->perchar[i].dheight+mv->perchar[i].kernafter;
	    MVDrawLine(mv,pixmap,0, yp,mv->vwidth,yp,
		    mv->type==mv_kernonly  && i!=mv->glyphcnt-1 ?kernlinecol :
		    mv->type==mv_widthonly                      ?rbearinglinecol :
			widthcol);
	}
	y += mv->perchar[i].yoff;
	bdfc = mv->bdf==NULL ?	BDFPieceMealCheck(mv->show,mv->glyphs[i].sc->orig_pos) :
				BDFGetMergedChar( mv->bdf->glyphs[mv->glyphs[i].sc->orig_pos]);
	if ( bdfc==NULL )
    continue;
	y += as-rint(iscale * bdfc->ymax);
	if ( mv->perchar[i].selected )
	    y += mv->activeoff;
	x = xbase - rint(iscale * (mv->pixelsize/2 + bdfc->xmin) - mv->perchar[i].xoff);
	width = bdfc->xmax-bdfc->xmin+1; height = bdfc->ymax-bdfc->ymin+1;
	if ( clip->y+clip->height<y )
    break;
	if ( y+iscale*height>=clip->y && x<clip->x+clip->width && x+iscale*width >= clip->x ) {
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
		    clut.clut[1] = selglyphcol;
	    } else {
		int scale, l;
		Color fg, bg;
		if ( mv->bdf!=NULL )
		    scale = BDFDepth(mv->bdf);
		else
		    scale = BDFDepth(mv->show);
		base.image_type = it_index;
		clut.clut_len = 1<<scale;
		bg = view_bgcol;
		fg = ( mv->perchar[i].selected ) ? selglyphcol : 0x000000;
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
	    if ( mv->pixelsize_set_by_window || mv->scale_index==SCALE_INDEX_NORMAL )
		GDrawDrawGlyph(pixmap,&gi,NULL,x,y);
	    else
		GDrawDrawImageMagnified(pixmap, &gi, NULL, x,y,
			(int) rint((width*mv_scales[mv->scale_index])),
			(int) rint((height*mv_scales[mv->scale_index])));
	}
	if ( mv->bdf!=NULL ) BDFCharFree( bdfc );
    }
    if ( si!=-1 && mv->bdf==NULL &&  MVShowGrid(mv) && mv->type==mv_kernwidth ) {
	y = mv->perchar[si].dy-mv->yoff;
	if ( si!=0 )
	    MVDrawLine(mv,pixmap,0,y,mv->vwidth,y,kernlinecol);
	y += mv->perchar[si].dheight+mv->perchar[si].kernafter;
	MVDrawLine(mv,pixmap,0,y,mv->vwidth,y,rbearinglinecol);
    }
}

static void MVSubExpose(MetricsView *mv, GWindow pixmap, GEvent *event) {
    GRect old, *clip;
    int x,y,ybase, width,height, i;
    BDFChar *bdfc;
    struct _GImage base;
    GImage gi;
    GClut clut;
    int si;
    double iscale = mv->pixelsize_set_by_window ? 1.0 : mv_scales[mv->scale_index];
    double s = sin(-mv->sf->italicangle*3.1415926535897932/180.);
    int x_iaoffh = 0, x_iaoffl = 0;

    clip = &event->u.expose.rect;
    GDrawPushClip(pixmap,clip,&old);
    GDrawSetLineWidth(pixmap,0);

    if ( mv->vertical ) {
	MVSubVExpose(mv,pixmap,event);
	GDrawPopClip(pixmap,&old);
return;
    }

    ybase = mv->ybaseline - mv->yoff;

    if ( mv->showgrid )
	GDrawDrawLine(pixmap,0,ybase,mv->dwidth,ybase,widthcol);

    if ( mv->bdf==NULL && MVShowGrid(mv) ) {
	x = mv->perchar[0].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->vwidth - x - mv->perchar[0].dwidth - mv->perchar[0].kernafter;
	MVDrawLine(mv,pixmap,x,0,x,mv->vheight,widthcol);
	x_iaoffh = rint(ybase*s), x_iaoffl = rint((mv->vheight-ybase)*s);
	if ( ItalicConstrained && x_iaoffh!=0 ) {
	    MVDrawLine(mv,pixmap,x+x_iaoffh,0,x-x_iaoffl,mv->vheight,italicwidthcol);
	}
    }
    si = -1;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	if ( mv->perchar[i].selected ) si = i;
	x = mv->perchar[i].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->vwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
	if ( mv->bdf==NULL && MVShowGrid(mv) ) {
	    int xp = x+mv->perchar[i].dwidth+mv->perchar[i].kernafter;
	    MVDrawLine(mv,pixmap,xp, 0,xp,mv->vheight,
		    mv->type==mv_kernonly  && i!=mv->glyphcnt-1 ?kernlinecol :
		    mv->type==mv_widthonly                      ?rbearinglinecol :
			widthcol);
	    if ( ItalicConstrained && x_iaoffh!=0 ) {
		MVDrawLine(mv,pixmap,xp+x_iaoffh,0,xp-x_iaoffl,mv->vheight,italicwidthcol);
	    }
	}
	if ( mv->right_to_left )
	    x += mv->perchar[i].kernafter-mv->perchar[i].xoff;
	else
	    x += mv->perchar[i].xoff;
	bdfc = mv->bdf==NULL ?	BDFPieceMealCheck(mv->show,mv->glyphs[i].sc->orig_pos) :
				BDFGetMergedChar( mv->bdf->glyphs[mv->glyphs[i].sc->orig_pos]);
	if ( bdfc==NULL )
    continue;
	x += rint( iscale * bdfc->xmin );
	if ( mv->perchar[i].selected )
	    x += mv->activeoff;
	y = ybase - rint( iscale * bdfc->ymax ) - (iscale-1) - mv->perchar[i].yoff;
	width = bdfc->xmax-bdfc->xmin+1; height = bdfc->ymax-bdfc->ymin+1;
	if ( !mv->right_to_left && clip->x+clip->width<x )
    break;
	if ( x+rint(iscale*width)>=clip->x && y<clip->y+clip->height && y+rint(iscale*height) >= clip->y ) {
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
		    clut.clut[1] = selglyphcol;
	    } else {
		int lscale = 3000/mv->pixelsize, l;
		Color fg, bg;
		int scale;
		if ( mv->bdf!=NULL )
		    lscale = BDFDepth(mv->bdf);
		else
		    lscale = BDFDepth(mv->show);
		base.image_type = it_index;
		scale = lscale==8?256:lscale==4?16:4;
		clut.clut_len = scale;
		bg = view_bgcol;
		fg = ( mv->perchar[i].selected ) ? selglyphcol : 0x000000;
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
	    if ( mv->pixelsize_set_by_window || mv->scale_index==SCALE_INDEX_NORMAL )
		GDrawDrawGlyph(pixmap,&gi,NULL,x,y);
	    else
		GDrawDrawImageMagnified(pixmap, &gi, NULL, x,y,
			(int) rint((width*mv_scales[mv->scale_index])),
			(int) rint((height*mv_scales[mv->scale_index])));
	}
	if ( mv->bdf!=NULL ) BDFCharFree( bdfc );
    }
    if ( si!=-1 && mv->bdf==NULL && MVShowGrid(mv) && mv->type==mv_kernwidth ) {
	x = mv->perchar[si].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->vwidth - x;
	if ( si!=0 )
	    MVDrawLine(mv,pixmap,x,0,x,mv->vheight,kernlinecol);
	if ( mv->right_to_left )
	    x -= mv->perchar[si].dwidth+mv->perchar[si].kernafter;
	else
	    x += mv->perchar[si].dwidth+mv->perchar[si].kernafter;
	 MVDrawLine(mv,pixmap,x, 0,x,mv->vheight,rbearinglinecol);
    }
    GDrawPopClip(pixmap,&old);
}

static void MVExpose(MetricsView *mv, GWindow pixmap, GEvent *event) {
    GRect old, *clip;
    int x;
    int ke = mv->height-mv->sbh-(mv->fh+4);

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
    GDrawPopClip(pixmap,&old);
}

static void MVSetSubtables(SplineFont *sf) {
    GTextInfo **ti;
    OTLookup *otl;
    struct lookup_subtable *sub;
    int cnt, doit;
    MetricsView *mvs;
    int selected;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    /* There might be more than one metricsview wandering around. Update them all */
    for ( mvs = sf->metrics; mvs!=NULL; mvs=mvs->next ) {
	selected = false;
	for ( doit = 0; doit<2; ++doit ) {
	    cnt = 0;
	    for ( otl=sf->gpos_lookups; otl!=NULL; otl=otl->next ) {
		if ( otl->lookup_type == gpos_pair && FeatureTagInFeatureScriptList(
			    mvs->vertical?CHR('v','k','r','n') : CHR('k','e','r','n'),
			    otl->features)) {
		    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
			if ( doit ) {
			    ti[cnt] = calloc(1,sizeof(GTextInfo));
			    ti[cnt]->text = utf82u_copy(sub->subtable_name);
			    ti[cnt]->userdata = sub;
			    if ( sub==mvs->cur_subtable )
				ti[cnt]->selected = selected = true;
			    ti[cnt]->disabled = sub->kc!=NULL;
			    ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
			}
			++cnt;
		    }
		}
	    }
	    if ( !doit )
		ti = calloc(cnt+3,sizeof(GTextInfo *));
	    else {
		if ( cnt!=0 ) {
		    ti[cnt] = calloc(1,sizeof(GTextInfo));
		    ti[cnt]->line = true;
		    ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
		    ++cnt;
		}
		ti[cnt] = calloc(1,sizeof(GTextInfo));
		ti[cnt]->text = utf82u_copy(_("New Lookup Subtable..."));
		ti[cnt]->userdata = NULL;
		ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
		ti[cnt]->selected = !selected;
		++cnt;
		ti[cnt] = calloc(1,sizeof(GTextInfo));
	    }
	}
	if ( !selected )
	    mvs->cur_subtable = NULL;

	GGadgetSetList(mvs->subtable_list,ti,false);
    }
}

static void MVSetFeatures(MetricsView *mv) {
    SplineFont *sf = mv->sf;
    int i, j, cnt;
    GTextInfo **ti=NULL;
    uint32 *tags = NULL, script, lang;
    char buf[16];
    uint32 *stds;
    const unichar_t *pt = _GGadgetGetTitle(mv->script);

    if ( sf->cidmaster ) sf=sf->cidmaster;

    script = DEFAULT_SCRIPT;
    lang = DEFAULT_LANG;
    if ( u_strlen(pt)>=4 )
	script = (pt[0]<<24) | (pt[1]<<16) | (pt[2]<<8) | pt[3];
    if ( pt[4]=='{' && u_strlen(pt)>=9 )
	lang = (pt[5]<<24) | (pt[6]<<16) | (pt[7]<<8) | pt[8];
    if ( (uint32)mv->oldscript!=script || (uint32)mv->oldlang!=lang )
	stds = StdFeaturesOfScript(script);
    else {		/* features list may have changed, but retain those set */
	int32 len, sc;
	ti = GGadgetGetList(mv->features,&len);
	stds = malloc((len+1)*sizeof(uint32));
	for ( i=sc=0; i<len; ++i ) if ( ti[i]->selected )
	    stds[sc++] = (uint32) (intpt) ti[i]->userdata;
	stds[sc] = 0;
    }

    tags = SFFeaturesInScriptLang(sf,-2,script,lang);
    /* Never returns NULL */
    for ( cnt=0; tags[cnt]!=0; ++cnt );

    /*qsort(tags,cnt,sizeof(uint32),tag_comp);*/ /* The glist will do this for us */

    ti = malloc((cnt+2)*sizeof(GTextInfo *));
    for ( i=0; i<cnt; ++i ) {
	ti[i] = calloc( 1,sizeof(GTextInfo));
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	if ( (tags[i]>>24)<' ' || (tags[i]>>24)>0x7e )
	    sprintf( buf, "<%d,%d>", tags[i]>>16, tags[i]&0xffff );
	else {
	    buf[0] = tags[i]>>24; buf[1] = tags[i]>>16; buf[2] = tags[i]>>8; buf[3] = tags[i]; buf[4] = 0;
	}
	ti[i]->text = uc_copy(buf);
	ti[i]->userdata = (void *) (intpt) tags[i];
	for ( j=0; stds[j]!=0; ++j ) {
	    if ( stds[j] == tags[i] ) {
		ti[i]->selected = true;
	break;
	    }
	}
    }
    ti[i] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(mv->features,ti,false);
    mv->oldscript = script; mv->oldlang = lang;
}

static void MVSelectSubtable(MetricsView *mv, struct lookup_subtable *sub) {
    int32 len; int i;
    GTextInfo **old = GGadgetGetList(mv->subtable_list,&len);

    for ( i=0; i<len && (old[i]->userdata!=sub || old[i]->line); ++i )
    {
	// nothing //
    }
//    printf("MVSelectSubtable() i:%d sub:%p\n", i, sub );
    GGadgetSelectOneListItem(mv->subtable_list,i);
    if ( sub )
	mv->cur_subtable = sub;
}

static void MVRedrawI(MetricsView *mv,int i,int oldxmin,int oldxmax) {
    GRect r;
    BDFChar *bdfc;
    int off = 0;

    if ( mv->right_to_left || mv->vertical ) {
	/* right to left clipping is hard to think about, it doesn't happen */
	/*  often enough (I think) for me to put the effort to make it efficient */
	GDrawRequestExpose(mv->v,NULL,false);
return;
    }
    if ( mv->perchar[i].selected )
	off = mv->activeoff;
    r.y = 0; r.height = mv->vheight;
    r.x = mv->perchar[i].dx-mv->xoff; r.width = mv->perchar[i].dwidth;
    if ( mv->perchar[i].kernafter>0 )
	r.width += mv->perchar[i].kernafter;
    if ( mv->perchar[i].xoff<0 ) {
	r.x += mv->perchar[i].xoff;
	r.width -= mv->perchar[i].xoff;
    } else
	r.width += mv->perchar[i].xoff;
    bdfc = mv->bdf==NULL ?  BDFPieceMealCheck(mv->show,mv->glyphs[i].sc->orig_pos) :
			    mv->bdf->glyphs[mv->glyphs[i].sc->orig_pos];
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
    GDrawRequestExpose(mv->v,&r,false);
    if ( mv->perchar[i].selected && i!=0 ) {
	struct lookup_subtable *sub = mv->glyphs[i].kp!=NULL ? mv->glyphs[i].kp->subtable : mv->glyphs[i].kc!=NULL && mv->glyphs[i].kc->offsets[mv->glyphs[i].kc_index]!=0 ? mv->glyphs[i].kc->subtable : NULL;
	if ( sub!=NULL )
	    MVSelectSubtable(mv,sub);
    }
}

static void MVDeselectChar(MetricsView *mv, int i) {

    mv->perchar[i].selected = false;
    if ( mv->perchar[i].name!=NULL )
	GGadgetSetEnabled(mv->perchar[i].name,mv->bdf==NULL);
    MVRedrawI(mv,i,0,0);
}

static void MVSelectSubtableForScript(MetricsView *mv,uint32 script) {
    int32 len;
    GTextInfo **ti = GGadgetGetList(mv->subtable_list,&len);
    struct lookup_subtable *sub;
    int i;

    if ( mv->cur_subtable != NULL && FeatureScriptTagInFeatureScriptList(
	    mv->vertical?CHR('v','k','r','n') : CHR('k','e','r','n'),script,
	    mv->cur_subtable->lookup->features ))
return;

    sub = NULL;
    for ( i=0; i<len; ++i )
	if ( ti[i]->userdata!=NULL &&
		FeatureScriptTagInFeatureScriptList(
		    mv->vertical?CHR('v','k','r','n') : CHR('k','e','r','n'),
		    script,((struct lookup_subtable *) (ti[i]->userdata))->lookup->features)) {
	    sub = ti[i]->userdata;
    break;
	}
    if ( sub!=NULL )
	MVSelectSubtable(mv,sub);
}


static void MVSelectChar(MetricsView *mv, int i) {

    if ( i>=mv->glyphcnt || i<0 )
return;
    mv->perchar[i].selected = true;
    if ( mv->perchar[i].name!=NULL )
	GGadgetSetEnabled(mv->perchar[i].name,false);
    if ( mv->glyphs[i].kp!=NULL )
	MVSelectSubtable(mv,mv->glyphs[i].kp->subtable);
    else if ( mv->glyphs[i].kc!=NULL && mv->glyphs[i].kc->offsets[mv->glyphs[i].kc_index]!=0 )
	MVSelectSubtable(mv,mv->glyphs[i].kc->subtable);
    else
	MVSelectSubtableForScript(mv,SCScriptFromUnicode(mv->glyphs[i].sc));
    MVRedrawI(mv,i,0,0);
}

static void MVDoSelect(MetricsView *mv, int i) {
    int j;

    for ( j=0; j<mv->glyphcnt; ++j )
	if ( j!=i && mv->perchar[j].selected )
	    MVDeselectChar(mv,j);
    MVSelectChar(mv,i);
}

static void MVSelectSetForAll(MetricsView *mv, int selected )
{
    int i;

    for ( i=0 ; i<mv->glyphcnt; ++i )
    {
	if( selected )
	    MVSelectChar(mv,i);
	else
	    MVDeselectChar(mv,i);
    }
}

void MVRefreshChar(MetricsView *mv, SplineChar *sc) {
    int i;
    for ( i=0; i<mv->glyphcnt; ++i ) if ( mv->glyphs[i].sc == sc )
	MVRedrawI(mv,i,0,0);
}

static void MVRefreshValues(MetricsView *mv, int i) {
    char buf[40];
    DBounds bb;
    SplineChar *sc = mv->glyphs[i].sc;
    int kern_offset;

    SplineCharFindBounds(sc,&bb);

    if( !mv->perchar[i].name )
	return;
    GGadgetSetTitle8(mv->perchar[i].name,sc->name);

if( !mv->perchar[i].width )
return;

    //printf("MVRefreshValues() **** setting width to %d\n", sc->width );
    sprintf(buf,"%d",mv->vertical ? sc->vwidth : sc->width);
    GGadgetSetTitle8(mv->perchar[i].width,buf);

    sprintf(buf,"%.2f",mv->vertical ? sc->parent->ascent-(double) bb.maxy : (double) bb.minx);
    if ( buf[strlen(buf)-1]=='0' ) {
	buf[strlen(buf)-1] = '\0';
	if ( buf[strlen(buf)-1]=='0' ) {
	    buf[strlen(buf)-1] = '\0';
	    if ( buf[strlen(buf)-1]=='.' )
		buf[strlen(buf)-1] = '\0';
	}
    }
    GGadgetSetTitle8(mv->perchar[i].lbearing,buf);

    sprintf(buf,"%.2f",(double) (mv->vertical ? sc->vwidth-(sc->parent->ascent-bb.miny) : sc->width-bb.maxx));
    if ( buf[strlen(buf)-1]=='0' ) {
	buf[strlen(buf)-1] = '\0';
	if ( buf[strlen(buf)-1]=='0' ) {
	    buf[strlen(buf)-1] = '\0';
	    if ( buf[strlen(buf)-1]=='.' )
		buf[strlen(buf)-1] = '\0';
	}
    }
    GGadgetSetTitle8(mv->perchar[i].rbearing,buf);

    kern_offset = 0x7ffffff;
    if ( mv->glyphs[i].kp!=NULL )
	kern_offset = mv->glyphs[i].kp->off;
    else if ( mv->glyphs[i].kc!=NULL )
	kern_offset = mv->glyphs[i].kc->offsets[ mv->glyphs[i].kc_index ];
if( !mv->perchar[i+1].kern )
  return;

    if ( kern_offset!=0x7ffffff && i!=mv->glyphcnt-1 ) {
	sprintf(buf,"%d",kern_offset);
	GGadgetSetTitle8(mv->perchar[i+1].kern,buf);
    } else if ( i!=mv->glyphcnt-1 )
	GGadgetSetTitle8(mv->perchar[i+1].kern,"");
}

static void MVMakeLabels(MetricsView *mv) {
    static GBox small = GBOX_EMPTY;
    GGadgetData gd;
    GTextInfo label;

    small.main_background = small.main_foreground = COLOR_DEFAULT;
    memset(&gd,'\0',sizeof(gd));
    memset(&label,'\0',sizeof(label));

    mv->mwidth = GGadgetScale(60);
    mv->displayend = mv->height- mv->sbh - 5*(mv->fh+4);
    /* We might not have set mv->vheight-2 yet */
    if ( mv->pixelsize_set_by_window )
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

static int MV_KernChanged(GGadget *g, GEvent *e);
static int MV_RBearingChanged(GGadget *g, GEvent *e);
static int MV_LBearingChanged(GGadget *g, GEvent *e);
static int MV_WidthChanged(GGadget *g, GEvent *e);

static void MVCreateFields(MetricsView *mv,int i) {
    static GBox small = GBOX_EMPTY;
    GGadgetData gd;
    GTextInfo label;
    static unichar_t nullstr[1] = { 0 };
    int j;
    extern GBox _GGadget_gtextfield_box;
    int udaidx = 1; // we leave element zero to be NULL to allow bounds checking.

    small = _GGadget_gtextfield_box;
    small.flags = 0;
    small.border_type = bt_none;
    small.border_shape = bs_rect;
    small.border_width = 0;
    small.padding = 0;

    memset(&gd,'\0',sizeof(gd));
    memset(&label,'\0',sizeof(label));
    memset(mv->perchar[i].updownkparray,'\0',sizeof(GGadget*)*10);
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
    mv->perchar[i].updownkparray[udaidx++] = mv->perchar[i].width;

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_LBearingChanged;
    mv->perchar[i].lbearing = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);
    mv->perchar[i].updownkparray[udaidx++] = mv->perchar[i].lbearing;

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_RBearingChanged;
    mv->perchar[i].rbearing = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);
    mv->perchar[i].updownkparray[udaidx++] = mv->perchar[i].rbearing;

    if ( i!=0 ) {
	gd.pos.y += mv->fh+4;
	gd.pos.x -= mv->mwidth/2;
	gd.handle_controlevent = MV_KernChanged;
	mv->perchar[i].kern = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);
	if( i==1 ) {
	    mv->perchar[i-1].updownkparray[udaidx] = mv->perchar[i].kern;
	}
	mv->perchar[i].updownkparray[udaidx++] = mv->perchar[i].kern;

	if ( i>=mv->glyphcnt ) {
	    for ( j=mv->glyphcnt+1; j<=i ; ++ j )
		mv->perchar[j].dx = mv->perchar[j-1].dx;
	    mv->glyphcnt = i+1;
	}
    }

    GWidgetIndicateFocusGadget(mv->text);
}

static void MVSetSb(MetricsView *mv);
static int MVSetVSb(MetricsView *mv);

void MVRefreshMetric(MetricsView *mv) {
    double iscale = mv->pixelsize_set_by_window ? 1.0 : mv_scales[mv->scale_index];
    double scale = iscale*mv->pixelsize/(double) (mv->sf->ascent+mv->sf->descent);
    SplineFont *sf = mv->sf;
    int cnt;
    // Count the valid glyphs and segfault if there is no null splinechar terminator.
    for ( cnt=0; mv->glyphs[cnt].sc!=NULL; ++cnt );
    // Calculate positions.
    int x = 10; int y = 10;
    for ( int i=0; i<cnt; ++i ) {
	MVRefreshValues(mv,i);
	SplineChar * sc = mv->glyphs[i].sc;
	BDFChar * bdfc = mv->bdf!=NULL ? mv->bdf->glyphs[sc->orig_pos] : BDFPieceMealCheck(mv->show,sc->orig_pos);
	mv->perchar[i].dwidth = rint(iscale * bdfc->width);
	mv->perchar[i].dx = x;
	mv->perchar[i].xoff = rint(iscale * mv->glyphs[i].vr.xoff);
	mv->perchar[i].yoff = rint(iscale * mv->glyphs[i].vr.yoff);
	mv->perchar[i].kernafter = rint(iscale * mv->glyphs[i].vr.h_adv_off);
	x += mv->perchar[i].dwidth + mv->perchar[i].kernafter;

	mv->perchar[i].dheight = rint(sc->vwidth*scale);
	mv->perchar[i].dy = y;
	if ( mv->vertical ) {
	    mv->perchar[i].kernafter = rint( iscale * mv->glyphs[i].vr.v_adv_off);
	    y += mv->perchar[i].dheight + mv->perchar[i].kernafter;
	}
    }
    MVSetVSb(mv);
    MVSetSb(mv);
}

static void MVRemetric(MetricsView *mv) {
    SplineChar *anysc, *goodsc;
    int i, cnt, x, y, goodpos;
    const unichar_t *_script = _GGadgetGetTitle(mv->script);
    uint32 script, lang, *feats;
    char buf[20];
    int32 len;
    GTextInfo **ti;
    SplineChar *sc;
    BDFChar *bdfc;
    double iscale = mv->pixelsize_set_by_window ? 1.0 : mv_scales[mv->scale_index];
    double scale = iscale*mv->pixelsize/(double) (mv->sf->ascent+mv->sf->descent);
    SplineFont *sf;

    anysc = goodsc = NULL; goodpos = -1;
    // We recurse through all of the characters in the metrics view.
    for ( i=0; i<mv->clen && mv->chars[i] ; ++i ) {
        // We assign the first splinechar to anysc.
	if ( anysc==NULL ) anysc = mv->chars[i];
        // We assign the first splinechar of a non-default script to goodsc.
	if ( SCScriptFromUnicode(mv->chars[i])!=DEFAULT_SCRIPT ) {
	    goodsc = mv->chars[i];
	    goodpos = i;
    break;
	}
    }
    if ( _script[0]=='D' && _script[1]=='F' && _script[2]=='L' && _script[3]=='T' ) {
	if ( goodsc!=NULL ) {
	    /* Set the script */ /* Remember if we get here the script is DFLT */
            // To be clear, in this case, we set the script of the metrics view according to the first non-default script in the set of selected characters.
	    script = SCScriptFromUnicode(goodsc);
	    buf[0] = script>>24; buf[1] = script>>16; buf[2] = script>>8; buf[3] = script;
	    strcpy(buf+4,"{dflt}");
	    GGadgetSetTitle8(mv->script,buf);
	    MVSelectSubtableForScript(mv,script);
	    MVSetFeatures(mv);
	}
    } else {
	if ( anysc==NULL ) {
	    /* If we get here the script is not DFLT */
	    GGadgetSetTitle8(mv->script,"DFLT{dflt}");
            // Why do se set the title to DFLT then?
	    MVSetFeatures(mv);
	}
    }
    _script = _GGadgetGetTitle(mv->script);
    script = DEFAULT_SCRIPT; lang = DEFAULT_LANG;
    if ( u_strlen(_script)>=4 && (u_strchr(_script,'{')==NULL || u_strchr(_script,'{')-_script>=4)) {
        // If there is a four-character script identifier, pack it in script.
        // If there is a language identifier, pack that in lang.
	unichar_t *pt;
	script = (_script[0]<<24) | (_script[1]<<16) | (_script[2]<<8) | _script[3];
	if ( (pt = u_strchr(_script,'{'))!=NULL && u_strlen(pt+1)>=4 &&
		(u_strchr(pt+1,'}')==NULL || u_strchr(pt+1,'}')-(pt+1)>=4 ))
	    lang = (pt[1]<<24) | (pt[2]<<16) | (pt[3]<<8) | pt[4];
    }

    // Parse the current list of features into feats.
    ti = GGadgetGetList(mv->features,&len);
    for ( i=cnt=0; i<len; ++i )
	if ( ti[i]->selected ) ++cnt;
    feats = calloc(cnt+1,sizeof(uint32));
    for ( i=cnt=0; i<len; ++i )
	if ( ti[i]->selected )
	    feats[cnt++] = (intpt) ti[i]->userdata;

    // Regenerate glyphs for the selected characters according to features, script, and resolution.
    free(mv->glyphs); mv->glyphs = NULL;
    sf = mv->sf;
    if ( sf->cidmaster ) sf = sf->cidmaster;
    mv->glyphs = ApplyTickedFeatures(sf,feats,script, lang, mv->pixelsize, mv->chars);
    free(feats);
    if ( goodsc!=NULL )
	mv->right_to_left = SCRightToLeft(goodsc)?1:0;

    // Count the valid glyphs and segfault if there is no null splinechar terminator.
    for ( cnt=0; mv->glyphs[cnt].sc!=NULL; ++cnt );
    // If there are too many relative to the available rows, make space.
    if ( cnt>=mv->max ) {
	int oldmax=mv->max;
	mv->max = cnt+10;
	mv->perchar = realloc(mv->perchar,mv->max*sizeof(struct metricchar));
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }
    // Null names of controls in rows to be abandoned, starting at the last valid glyph and continuing to the end of mv->glyphs.
    // This may segfault here if mv->max is less than mv->glyphcnt, thus if cnt was 10 less than mv->glyphcnt.
    // It may segfault in GGadgetSetTitle if the gadgets do not exist.
    for ( i=cnt; i<mv->glyphcnt; ++i ) {
	static unichar_t nullstr[] = { 0 };
	GGadgetSetTitle(mv->perchar[i].name,nullstr);
	GGadgetSetTitle(mv->perchar[i].width,nullstr);
	GGadgetSetTitle(mv->perchar[i].lbearing,nullstr);
	GGadgetSetTitle(mv->perchar[i].rbearing,nullstr);
	if ( mv->perchar[i].kern!=NULL )
	    GGadgetSetTitle(mv->perchar[i].kern,nullstr);
    }
    // So we set mv->glyphcnt to something possibly less than the size of mv->glyphs.
    mv->glyphcnt = cnt;
    // We now populate any new rows with controls.
    for ( i=0; i<cnt; ++i ) {
	if ( mv->perchar[i].width==NULL ) {
	    MVCreateFields(mv,i);
	}
    }
    // Refresh.
    MVRefreshMetric(mv);
}

void MVReKern(MetricsView *mv) {
    MVRemetric(mv);
    GDrawRequestExpose(mv->v,NULL,false);
}

void MVRegenChar(MetricsView *mv, SplineChar *sc) {
    int i;

    if( !sc->suspendMetricsViewEventPropagation )
    {
	if ( mv->bdf==NULL && sc->orig_pos<mv->show->glyphcnt )
	{
	    BDFCharFree(mv->show->glyphs[sc->orig_pos]);
	    mv->show->glyphs[sc->orig_pos] = NULL;
	}
    }

    for ( i=0; i<mv->glyphcnt; ++i ) {
	MVRefreshValues(mv,i);
    }
    for ( i=0; i<mv->glyphcnt; ++i )
    {
	if ( mv->glyphs[i].sc == sc )
	    break;
    }
    if ( i>=mv->glyphcnt )
	return;		/* Not displayed */
    MVRemetric(mv);
    GDrawRequestExpose(mv->v,NULL,false);
}

static void MVChangeDisplayFont(MetricsView *mv, BDFFont *bdf) {
    int i;

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
	BDFFontFree(mv->show);
	mv->show = NULL;
    } else if ( bdf==NULL ) {
	BDFFontFree(mv->show);
	mv->show = SplineFontPieceMeal(mv->sf,mv->layer,mv->ptsize,mv->dpi,
				       MVGetSplineFontPieceMealFlags( mv ), NULL );
    }
    mv->bdf = bdf;
    MVRemetric(mv);
}

static int isValidInt(unichar_t *end) {
    if ( *end && !(*end=='-' && end[1]=='\0'))
	return 0;
    return 1;
}

/*
 * Unused
static int GGadgetToInt(GGadget *g)
{
    unichar_t *end;
    int val = u_strtol(_GGadgetGetTitle(g),&end,10);
    return val;
}
*/

static real GGadgetToReal(GGadget *g)
{
    unichar_t *end;
    real val = u_strtod(_GGadgetGetTitle(g),&end);
    return val;
}

static void MV_handle_collabclient_maybeSnapshot( MetricsView *mv, SplineChar *sc ) {
    if ( collabclient_inSessionFV(&mv->fv->b) ) {
	int dohints = 0;
	SCPreserveState( sc, dohints );
    }
}


/* If we are in a collab session, then send the redo through */
/* to the server to update other clients to our state.	     */
static void MV_handle_collabclient_sendRedo( MetricsView *mv, SplineChar *sc ) {
    if( collabclient_inSessionFV( &mv->fv->b ) ) {
	collabclient_sendRedo_SC( sc, UNDO_LAYER_UNKNOWN );

	/* CharViewBase* cv = sc->views; */
	/* if( !cv ) */
	/*     cv = CharViewCreate( sc, mv->fv, -1 ); */
	/* collabclient_sendRedo( cv ); */
    }
}

static int MV_WidthChanged(GGadget *g, GEvent *e) {
/* This routines called during "Advanced Width Metrics" viewing */
/* any time "Width" changed or screen is updated		*/
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( which>=mv->glyphcnt )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->glyphs[which].sc;
	if (!isValidInt(end))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && val!=sc->width ) {
//	    dumpUndoChain( "before SCPreserveWidth...", sc, &sc->layers[ly_fore].undoes );
	    SCPreserveWidth(sc);
	    if( collabclient_inSessionFV( &mv->fv->b ) ) {
		int dohints = 0;
		SCPreserveState( sc, dohints );
	    }

	    // set i to the correct column that has the active width gadget
	    for ( i=0; i<mv->glyphcnt; ++i ) {
		if ( mv->perchar[i].width == g )
		    break;
	    }

	    // Adjust the lbearing to consume or surrender half of the
	    // change that the width value is undergoing.
	    real offset = GGadgetToReal(mv->perchar[i].lbearing);
	    offset += (val - sc->width * 1.0)/2;
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[5] = 0;
	    DBounds bb;
	    SplineCharFindBounds(sc,&bb);
	    transform[4] = offset-bb.minx;
	    FVTrans( (FontViewBase *)mv->fv,sc,transform,NULL, 0 | fvt_alllayers );

	    SCSynchronizeWidth(sc,val,sc->width,NULL);
	    SCCharChangedUpdate(sc,ly_none);
	    printf("mv_widthChanged() sending collab\n");
	    MV_handle_collabclient_sendRedo(mv,sc);

	} else if ( mv->vertical && val!=sc->vwidth ) {
	    SCPreserveVWidth(sc);
	    sc->vwidth = val;
	    SCCharChangedUpdate(sc,ly_none);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->glyphcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int MV_LBearingChanged(GGadget *g, GEvent *e)
{
/* This routines called during "Advanced Width Metrics" viewing */
/* any time "LBrearing" changed or screen is updated		*/
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( which>=mv->glyphcnt )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	double val = u_strtod(_GGadgetGetTitle(g),&end);
	SplineChar *sc = mv->glyphs[which].sc;
	DBounds bb;
	SplineCharFindBounds(sc,&bb);
	if (!isValidInt(end))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && val!=bb.minx ) {
	    if ( collabclient_inSessionFV(&mv->fv->b) ) {
		int dohints = 0;
		SCPreserveState( sc, dohints );
	    }
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[5] = 0;
	    transform[4] = val-bb.minx;
	    FVTrans( (FontViewBase *)mv->fv,sc,transform,NULL,0 | fvt_alllayers );

//	    dumpUndoChain( "LBearing Changed...e", sc, &sc->layers[ly_fore].undoes );
	    MV_handle_collabclient_sendRedo(mv,sc);
	} else if ( mv->vertical && val!=sc->parent->ascent-bb.maxy ) {
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[4] = 0;
	    transform[5] = sc->parent->ascent-bb.maxy-val;
	    FVTrans( (FontViewBase *)mv->fv,sc,transform,NULL, fvt_dontmovewidth | fvt_alllayers );
	}

    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->glyphcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int MV_RBearingChanged(GGadget *g, GEvent *e) {
/* This routines called during "Advanced Width Metrics" viewing */
/* any time "RBrearing" changed or screen is updated		*/
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( which>=mv->glyphcnt )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtod(_GGadgetGetTitle(g),&end);
	SplineChar *sc = mv->glyphs[which].sc;
	DBounds bb;
	SplineCharFindBounds(sc,&bb);
	if (!isValidInt(end))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && rint(val+bb.maxx)!=sc->width ) {
	    int newwidth = rint(bb.maxx+val);
	    SCPreserveWidth(sc);
	    /* Width is an integer. Adjust the lbearing so that the rbearing */
	    /*  remains what was just typed in */
	    if ( newwidth!=bb.maxx+val ) {
		if ( collabclient_inSessionFV(&mv->fv->b) ) {
		    int dohints = 0;
		    SCPreserveState( sc, dohints );
		}
		real transform[6];
		transform[0] = transform[3] = 1.0;
		transform[1] = transform[2] = transform[5] = 0;
		transform[4] = newwidth-val-bb.maxx;
		FVTrans( (FontViewBase *)mv->fv,sc,transform,NULL,fvt_dontmovewidth);
	    }
	    SCSynchronizeWidth(sc,newwidth,sc->width,NULL);
	    SCCharChangedUpdate(sc,ly_none);

//	    dumpUndoChain( "RBearing Changed...2", sc, &sc->layers[ly_fore].undoes );
	    MV_handle_collabclient_sendRedo(mv,sc);
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
		FVTrans( (FontViewBase *)mv->fv,sc,transform,NULL,fvt_dontmovewidth);
	    }
	    SCCharChangedUpdate(sc,ly_none);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->glyphcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int AskNewKernClassEntry(SplineChar *fsc,SplineChar *lsc,int first_is_0,int second_is_0) {
    char *yesno[3];
    yesno[0] = _("_Alter Class");
    yesno[1] = _("_Create Pair");
    yesno[2] = NULL;
return( gwwv_ask(_("Use Kerning Class?"),(const char **) yesno,0,1,
	_("This kerning pair (%.20s and %.20s) is currently part of a kerning class with a 0 offset for this combination. Would you like to alter this kerning class entry (or create a kerning pair for just these two glyphs)?"),
	first_is_0  ? _("{Everything Else}") : fsc->name,
	second_is_0 ? _("{Everything Else}") : lsc->name)==0 );
}


static int MV_ChangeKerning(MetricsView *mv, int which, int offset, int is_diff) {
    SplineChar *sc = mv->glyphs[which].sc;
    SplineChar *psc = mv->glyphs[which-1].sc;
    KernPair *kp = 0;
    KernClass *kc; int index;
    int i;
    struct lookup_subtable *sub = GGadgetGetListItemSelected(mv->subtable_list)->userdata;
    double iscale = mv->pixelsize_set_by_window ? 1.0 : mv_scales[mv->scale_index];

    kp = mv->glyphs[which-1].kp;
    kc = mv->glyphs[which-1].kc;
    index = mv->glyphs[which-1].kc_index;

    if ( kc!=NULL ) {
	if ( index==-1 )
	    kc = NULL;
	else if ( kp != NULL && kp->off != 0 )
	    kc=NULL;
	else if ( (!is_diff && offset==kc->offsets[index]) ||
		  ( is_diff && offset==0))
return( true );		/* No change, don't bother user */
	/* If there is already a kerning pair, then assume it takes the precedence over the kerning class */
	else if ( kc->offsets[index]==0 && !AskNewKernClassEntry(psc,sc,mv->glyphs[which-1].prev_kc0,mv->glyphs[which-1].next_kc0))
	    kc=NULL;
	else
	    offset = kc->offsets[index] = is_diff ? kc->offsets[index]+offset : offset;
    }
    if ( kc==NULL ) {
	if ( sub!=NULL && sub->kc!=NULL ) {
	    /* If the subtable we were given contains a kern class, and for some reason */
	    /*  we can't, or don't want to, use that kern class, then see */
	    /*  if the lookup contains another subtable with no kern classes */
	    /*  and use that */
	    struct lookup_subtable *s;
	    for ( s = sub->lookup->subtables; s!=NULL && s->kc!=NULL; s=s->next );
	    sub = s;
	}
	if ( sub==NULL ) {
	    struct subtable_data sd;
	    memset(&sd,0,sizeof(sd));
	    sd.flags = (mv->vertical ? sdf_verticalkern : sdf_horizontalkern ) |
		    sdf_kernclass;
	    sub = SFNewLookupSubtableOfType(psc->parent,gpos_pair,&sd,mv->layer);
	    if ( sub==NULL )
return( false );
	    mv->cur_subtable = sub;
	    MVSetSubtables(mv->sf);
	    MVSetFeatures(mv);
	}

	/* If we change the kerning offset, then any pixel corrections*/
	/*  will no longer apply (they only had meaning with the old  */
	/*  offset) so free the device table, if any */
	if ( kp != NULL && ((!is_diff && kp->off!=offset) || ( is_diff && offset!=0)) ) {
	    DeviceTableFree(kp->adjust);
	    kp->adjust = NULL;
	}

	offset = is_diff && kp != NULL ? kp->off+offset : offset;
	/* If kern offset has been set to zero by user, then cleanup this kerning pair */
	if ( kp != NULL && offset == 0 ) {
	    KernPair *kpcur, *kpprev;
	    KernPair **kphead = mv->vertical ? &psc->vkerns : &psc->kerns;
	    if ( kp == *kphead ) {
		*kphead = kp->next;
	    } else {
		kpprev = *kphead;
		for ( kpcur=kpprev->next; kpcur != NULL; kpcur = kpcur->next ) {
		    if ( kpcur == kp ) {
			kpprev->next = kp->next;
		break;
		    }
		    kpprev = kpcur;
		}
	    }

	    // avoid dangling refrences to kp
	    int i = 0;
	    for( i=0; mv->glyphs[i].sc; i++ ) {
		if( i!=which && mv->glyphs[i].kp == kp ) {
		    mv->glyphs[i].kp = 0;
		}
	    }
	    chunkfree( kp,sizeof(KernPair) );
	    kp = mv->glyphs[which-1].kp = NULL;
	} else if ( offset != 0 ) {
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
		mv->glyphs[which-1].kp = kp;
	    }
	    kp->off = offset;
	    kp->subtable = sub;
	    if ( !mv->vertical )
		MMKern(sc->parent,psc,sc,is_diff?offset:offset-kp->off,sub,kp);
	}
    }
    int16 newkernafter = iscale * (offset*mv->pixelsize)/
	(mv->sf->ascent+mv->sf->descent);
    mv->perchar[which-1].kernafter = newkernafter;

    if ( mv->vertical ) {
	for ( i=which; i<mv->glyphcnt; ++i ) {
	    mv->perchar[i].dy = mv->perchar[i-1].dy+mv->perchar[i-1].dheight +
		    mv->perchar[i-1].kernafter ;
	}
    } else {
	for ( i=which; i<mv->glyphcnt; ++i ) {
	    mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth +
		    mv->perchar[i-1].kernafter;
	}
    }

    /**
     * Class based kerning. If we have altered one pair "Tc" then we
     * want to find any other pairs in the same class that are shown
     * and alter them in a similar way. Note that the update to the
     * value shown is already done as that is taken from the
     * KernClass. We can get away with just calling MVRefreshValues()
     * on the right indexes to update the kern value entry boxes. On
     * the other hand, we have to make sure the guide and glyph
     * display is adjusted accordingly too otherwise the user will not
     * see the currect kerning for all other digraphs in the same
     * class even thuogh the kerning entry box is updated.
     */
    if( kc && psc && sc )
    {
	// cache the cell in the kernclass that we are editing for quick comparison
	// in the loop
	int pscidx = KernClassFindIndexContaining( kc->firsts,  kc->first_cnt,  psc->name );
	int  scidx = KernClassFindIndexContaining( kc->seconds, kc->second_cnt,  sc->name );

	if( pscidx > 0 && scidx > 0 )
	{
	    for ( i=1; i<mv->glyphcnt; ++i )
	    {
		// don't check yourself.
		if( i-1 == which )
		    continue;

		/* printf("mv->glyphs[i-1].sc.name:%s\n", mv->glyphs[i-1].sc->name ); */
		/* printf("mv->glyphs[i  ].sc.name:%s\n", mv->glyphs[i  ].sc->name ); */

		int pidx = KernClassFindIndexContaining( kc->firsts,
							 kc->first_cnt,
							 mv->glyphs[i-1].sc->name );
		/*
		 * Same value for firsts in the kernclass matrix
		 */
		if( pidx == pscidx )
		{
		    int idx = KernClassFindIndexContaining( kc->seconds,
							    kc->second_cnt,
							    mv->glyphs[ i ].sc->name );

		    /*
		     * First and Second match, we have the same cell
		     * in the kernclass and thus the same kern value
		     * should be applied.
		     */
		    if( scidx == idx )
		    {
			// update the kern text entry box in the lower part of
			// the window.
			MVRefreshValues( mv, i-1 );

			//
			// Shift the guide and kerning for this digraph, and move
			// all the glyphs on the right over or back a bit so that things
			// still all fit as expected.
			//
			mv->perchar[i-1].kernafter = newkernafter;
			int j;
			for ( j=i; j<mv->glyphcnt; ++j ) {
			    mv->perchar[j].dx = mv->perchar[j-1].dx + mv->perchar[j-1].dwidth +
				mv->perchar[j-1].kernafter;
			}
		    }
		}
	    }
	}
    }

    // refresh other kerning input boxes if they are the same characters
    static int MV_ChangeKerning_Nested = 0;
    int refreshOtherPairEntries = true;
    if( !MV_ChangeKerning_Nested && refreshOtherPairEntries && mv->glyphs[0].sc )
    {
	int i = 1;
	for( ; mv->glyphs[i].sc; i++ )
	{
	    if( i != which
		&& sc  == mv->glyphs[i].sc
		&& psc == mv->glyphs[i-1].sc )
	    {
		
		GGadget *g = mv->perchar[i].kern;
		unichar_t *end;
		int val = u_strtol(_GGadgetGetTitle(g),&end,10);

		MV_ChangeKerning_Nested = 1;
		int which = (intpt) GGadgetGetUserData(g);
		MV_ChangeKerning( mv, which, offset, is_diff );
		GGadgetSetTitle8( g, tostr(offset) );
		MV_ChangeKerning_Nested = 0;
	    }
	}
    }

    mv->sf->changed = true;
    GDrawRequestExpose(mv->v,NULL,false);

return( true );
}

static int MV_KernChanged(GGadget *g, GEvent *e) {
/* This routines called during "Advanced Width Metrics" viewing */
/* any time "Kern:" changed or screen is updated		*/
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( which>mv->glyphcnt-1 || which==0 )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);

	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else {
	    MV_ChangeKerning(mv,which,val, false);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->glyphcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }

    if( haveClassBasedKerningInView(mv) )
    {
	MVRefreshMetric(mv);
	GDrawRequestExpose(mv->v,NULL,false);
    }

return( true );
}

static void MVToggleVertical(MetricsView *mv) {
    int size;

    mv->vertical = !mv->vertical;

    GGadgetSetTitle8( mv->widthlab, mv->vertical ? "Height:" : "Width:" );
    GGadgetSetTitle8( mv->lbearinglab, mv->vertical ? "TBearing:" : "LBearing:" );
    GGadgetSetTitle8( mv->rbearinglab, mv->vertical ? "BBearing:" : "RBearing:" );
    GGadgetSetTitle8( mv->kernlab, mv->vertical ? "VKern:" : "Kern:" );

    if ( mv->vertical )
	if ( mv->scale_index<4 ) mv->scale_index = 4;

    if ( mv->pixelsize_set_by_window ) {
	size = (mv->displayend - mv->topend - 4);
	if ( mv->dwidth-20<size )
	    size = mv->dwidth-20;
	size *= mv_scales[mv->scale_index];
	if ( mv->pixelsize != size ) {
	    mv->pixelsize = size;
	    mv->dpi = 72;
	    if ( mv->bdf==NULL ) {
		BDFFontFree(mv->show);
		mv->show = SplineFontPieceMeal(mv->sf,mv->layer,mv->pixelsize,72,
					       MVGetSplineFontPieceMealFlags( mv ), NULL );
	    }
	    MVRemetric(mv);
	}
    }
}

static SplineChar *MVSCFromUnicode(MetricsView *mv, SplineFont *sf, EncMap *map, int ch,BDFFont *bdf) {
    int i;
    SplineChar *sc;

    if ( mv->fake_unicode_base && ch>=mv->fake_unicode_base &&
	    ch<=mv->fake_unicode_base+mv->sf->glyphcnt )
return( mv->sf->glyphs[ch-mv->fake_unicode_base] );

    i = SFFindSlot(sf,map,ch,NULL);
    if ( i==-1 )
return( NULL );
    else {
	sc = SFMakeChar(sf,map,i);
	if ( bdf!=NULL )
	    BDFMakeChar(bdf,map,i);
    }
return( sc );
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

    for ( i=mv->coff; i<mv->glyphcnt; ++i ) {
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

    GScrollBarSetBounds(mv->hsb,0,mv->glyphcnt,cnt);
    GScrollBarSetPos(mv->hsb,mv->coff);
}

static int MVSetVSb(MetricsView *mv) {
    int max, min, ret, yoff;
    int fudge;

    if ( mv->displayend==0 )
return(0);		/* Setting the scroll bar is premature */

    if ( mv->vertical ) {
	min = max = 0;
	if ( mv->glyphcnt!=0 )
	    max = mv->perchar[mv->glyphcnt-1].dy + mv->perchar[mv->glyphcnt-1].dheight;
	fudge = 10;
    } else {
	SplineFont *sf = mv->sf;
	int pixels = mv->pixelsize_set_by_window ? mv->vheight : mv->pixelsize;
	fudge = pixels/4;
	min = -(pixels*sf->descent)/(sf->ascent+sf->descent);
	max = pixels + min;
	min *= mv_scales[mv->scale_index];
	max *= mv_scales[mv->scale_index];
    }
    mv->ybaseline = max;
    max += fudge*mv_scales[mv->scale_index] + mv->vheight;
    min -= fudge*mv_scales[mv->scale_index];
    GScrollBarSetBounds(mv->vsb,min,max,mv->vheight);
    yoff = mv->yoff;
    if ( yoff+mv->vheight > max )
	yoff = max - mv->vheight;
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
        newpos = mv->glyphcnt-cnt;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>mv->glyphcnt-cnt )
        newpos = mv->glyphcnt-cnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=mv->coff ) {
	int old = mv->coff;
	int diff = newpos-mv->coff;
	int charsize = mv->perchar[newpos].dx-mv->perchar[old].dx;
	GRect fieldrect, charrect;

	mv->coff = newpos;
	charrect.x = 0; charrect.width = mv->vwidth;
	charrect.y = 0; charrect.height = mv->vheight;
	fieldrect.x = mv->mbase+mv->mwidth; fieldrect.width = mv->width-mv->mbase;
	fieldrect.y = mv->displayend; fieldrect.height = mv->height-mv->sbh-mv->displayend;
	GScrollBarSetBounds(mv->hsb,0,mv->glyphcnt,cnt);
	GScrollBarSetPos(mv->hsb,mv->coff);
	MVMoveFieldsBy(mv,newpos*mv->mwidth);
	GDrawScroll(mv->gw,&fieldrect,-diff*mv->mwidth,0);
	mv->xoff = mv->perchar[newpos].dx-mv->perchar[0].dx;
	if ( mv->right_to_left ) {
	    charsize = -charsize;
	}
	GDrawScroll(mv->v,&charrect,-charsize,0);
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
	charrect.x = 0; charrect.width = mv->vwidth;
	charrect.y = 0; charrect.height = mv->vheight;
	GScrollBarSetPos(mv->vsb,mv->yoff);
	GDrawScroll(mv->v,&charrect,0,diff);
    }
}

static int MVFakeUnicodeOfSc(MetricsView *mv, SplineChar *sc) {

    if ( sc->unicodeenc!=-1 )
return( sc->unicodeenc );

    if ( mv->fake_unicode_base==0 ) {		/* Not set */
	/* If they have nothing in Supplementary Private Use Area-A use it */
	/* If they have nothing in Supplementary Private Use Area-B use it */
	/* else just use 0xfffd */
	int a, al, ah, b, bl, bh;
	int gid,k,max;
	SplineChar *test;
	SplineFont *_sf, *sf;
	sf = mv->sf;
	if ( sf->cidmaster ) sf = sf->cidmaster;
	k=0;
	a = al = ah = b = bl = bh = 0;
	max = 0;
	do {
	    _sf =  ( sf->subfontcnt==0 ) ? sf : sf->subfonts[k];
	    for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (test=_sf->glyphs[gid])!=NULL ) {
		if ( test->unicodeenc>=0xf0000 && test->unicodeenc<=0xfffff ) {
		    a = true;
		    if ( test->unicodeenc<0xf8000 )
			al = true;
		    else
			ah = true;
		} else if ( test->unicodeenc>=0x100000 && test->unicodeenc<=0x10ffff ) {
		    b = true;
		    if ( test->unicodeenc<0x108000 )
			bl = true;
		    else
			bh = true;
		}
	    }
	    if ( gid>max ) max = gid;
	    ++k;
	} while ( k<sf->subfontcnt );
	if ( !a )		/* Nothing in SPUA-A */
	    mv->fake_unicode_base = 0xf0000;
	else if ( !b )
	    mv->fake_unicode_base = 0x100000;
	else if ( max<0x8000 ) {
	    if ( !al )
		mv->fake_unicode_base = 0xf0000;
	    else if ( !ah )
		mv->fake_unicode_base = 0xf8000;
	    else if ( !bl )
		mv->fake_unicode_base = 0x100000;
	    else if ( !bh )
		mv->fake_unicode_base = 0x108000;
	}
	if ( mv->fake_unicode_base==0 )
	    mv->fake_unicode_base = -1;
    }

    if ( mv->fake_unicode_base==-1 )
return( 0xfffd );
    else
return( mv->fake_unicode_base+sc->orig_pos );
}

static int MVOddMatch(MetricsView *mv,int uni,SplineChar *sc) {
    if ( sc->unicodeenc!=-1 )
return( false );
    else if ( mv->fake_unicode_base<=0 )
return( uni==0xfffd );
    else
return( uni>=mv->fake_unicode_base && sc->orig_pos == uni-mv->fake_unicode_base );
}

void MVSetSCs(MetricsView *mv, SplineChar **scs) {
    /* set the list of characters being displayed to those in scs */
    int len;
    unichar_t *ustr;

    for ( len=0; scs[len]!=NULL; ++len );
    if ( len>=mv->cmax )
	mv->chars = realloc(mv->chars,(mv->cmax=len+10)*sizeof(SplineChar *));
    memcpy(mv->chars,scs,(len+1)*sizeof(SplineChar *));
    mv->clen = len;

    ustr = malloc((len+1)*sizeof(unichar_t));
    for ( len=0; scs[len]!=NULL; ++len )
	if ( scs[len]->unicodeenc>0 )
	    ustr[len] = scs[len]->unicodeenc;
	else
	    ustr[len] = MVFakeUnicodeOfSc(mv,scs[len]);
    ustr[len] = 0;
    GGadgetSetTitle(mv->text,ustr);
    free(ustr);

    MVRemetric(mv);

    GDrawRequestExpose(mv->v,NULL,false);
}


static int WordlistEscapedInputStringToRealString_getFakeUnicodeAs_MVFakeUnicodeOfSc( SplineChar *sc, void* udata )
{
    MetricsView *mv = (MetricsView *)udata;
    int n = MVFakeUnicodeOfSc( mv, sc );
    return n;
}


static void MVTextChanged(MetricsView *mv) {
    const unichar_t *ret = 0, *pt, *ept, *tpt;
    int i,ei, j, start=0, end=0;
    int missing;
    int direction_change = false;
    SplineChar **hold = NULL;

    ret = _GGadgetGetTitle(mv->text);

    // convert the slash escpae codes and the like to the real string we will use
    // for the metrics window
    WordListLine wll = WordlistEscapedInputStringToParsedDataComplex(
    	mv->sf, _GGadgetGetTitle(mv->text),
    	WordlistEscapedInputStringToRealString_getFakeUnicodeAs_MVFakeUnicodeOfSc, mv );
    ret = WordListLine_toustr( wll );

    if (( ret[0]<0x10000 && isrighttoleft(ret[0]) && !mv->right_to_left ) ||
	    ( ret[0]<0x10000 && !isrighttoleft(ret[0]) && mv->right_to_left )) {
	direction_change = true;
	mv->right_to_left = !mv->right_to_left;
    }
    for ( pt=ret, i=0; i<mv->clen && *pt!='\0'; ++i, ++pt )
	if ( *pt!=mv->chars[i]->unicodeenc &&
		!MVOddMatch(mv,*pt,mv->chars[i]))
    break;
    if ( i==mv->clen && *pt=='\0' )
return;					/* Nothing changed */
    for ( ept=ret+u_strlen(ret)-1, ei=mv->clen-1; ; --ei, --ept )
	if ( ei<0 || ept<ret || (*ept!=mv->chars[ei]->unicodeenc &&
		!MVOddMatch(mv,*ept,mv->chars[ei]))) {
	    ++ei; ++ept;
    break;
	} else if ( ei<i || ept<pt ) {
	    ++ei; ++ept;
    break;
	}
    if ( ei==i && ept==pt )
	IError("No change when there should have been one in MV_TextChanged");
    if ( u_strlen(ret)>=mv->cmax ) {
	int oldmax=mv->cmax;
	mv->cmax = u_strlen(ret)+10;
	mv->chars = realloc(mv->chars,mv->cmax*sizeof(SplineChar *));
	memset(mv->chars+oldmax,'\0',(mv->cmax-oldmax)*sizeof(SplineChar *));
    }

    missing = 0;
    for ( tpt=pt; tpt<ept; ++tpt )
	if ( mv->fake_unicode_base>0 && *tpt>=mv->fake_unicode_base &&
		*tpt<=mv->fake_unicode_base+mv->sf->glyphcnt )
	    /* That's ok */;
	else if ( SFFindSlot(mv->sf,mv->fv->b.map,*tpt,NULL)==-1 )
	    ++missing;

    if ( ept-pt-missing > ei-i ) {
	if ( i<mv->clen ) {
	    int diff = (ept-pt-missing) - (ei-i);
	    hold = malloc((mv->clen+diff+6)*sizeof(SplineChar *));
	    for ( j=mv->clen-1; j>=ei; --j )
		hold[j+diff] = mv->chars[j];
	    start = ei+diff; end = mv->clen+diff;
	}
    } else if ( ept-pt-missing != ei-i ) {
	int diff = (ept-pt-missing) - (ei-i);
	for ( j=ei; j<mv->clen; ++j )
	    if ( j+diff>=0 )
		mv->chars[j+diff] = mv->chars[j];
    }
    for ( j=i; pt<ept; ++pt ) {
	SplineChar *sc;
	sc = MVSCFromUnicode(mv,mv->sf,mv->fv->b.map,*pt,mv->bdf);
	if ( sc!=NULL )
	    mv->chars[j++] = sc;
    }
    if ( hold!=NULL ) {
	/* We had to figure out what sc's there were before we wrote over them*/
	/*  but we couldn't put them where they belonged until everything before*/
	/*  them was set properly */
	for ( j=start; j<end; ++j )
	    mv->chars[j] = hold[j];
	free(hold);
    }
    mv->clen = u_strlen(ret)-missing;
    mv->chars[mv->clen] = NULL;
    MVRemetric(mv);

    // handle selecting the default glyph if desired this is slightly
    // complex because we need to handle when there is no selected
    // entry, which is the case just after loading a word list, and
    // then the first line is the right line.
    GTextInfo* gt = GGadgetGetListItemSelected(mv->text);
    if( !gt )
    {
	GTextInfo **ti=NULL;
	int32 len;
	ti = GGadgetGetList(mv->text,&len);
	if( len )
	    gt = ti[0];
    }

    selectUserChosenWordListGlyphs( mv, wll );
    GDrawRequestExpose(mv->v,NULL,false);
}

GTextInfo mv_text_init[] = {
    { (unichar_t *) "", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { NULL, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0'},
    { (unichar_t *) N_("Load Word List..."), NULL, 0, 0, (void *) -1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Load Glyph Name List..."), NULL, 0, 0, (void *) -2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

static void MVFigureGlyphNames(MetricsView *mv,const unichar_t *names) {
    char buffer[400], *pt, *start;
    SplineChar *founds[40];
    int i,cnt,ch;
    unichar_t *newtext;

    u2utf8_strcpy(buffer,names);
    start = buffer;
    for ( i=0; *start; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	if ( i>=40 )
    break;
	if ( (founds[i]=SFGetChar(mv->sf,-1,start))!=NULL )
	    ++i;
	*pt = ch;
	start = pt;
    }
    cnt = i;

    if ( cnt>=mv->cmax ) {
	mv->cmax = mv->clen+cnt+10;
	mv->chars = realloc(mv->chars,mv->cmax*sizeof(SplineChar *));
    }
    newtext = malloc((cnt+1)*sizeof(unichar_t));
    for ( i=0; i<cnt; ++i ) {
	newtext[i] = founds[i]->unicodeenc==-1 ?
						MVFakeUnicodeOfSc(mv,founds[i]) :
						founds[i]->unicodeenc;
	mv->chars[i] = founds[i];
    }
    newtext[i] = 0;
    mv->chars[i] = NULL;
    mv->clen = cnt;
    MVRemetric(mv);

    GGadgetSetTitle(mv->text,newtext);
    free(newtext);

    GDrawRequestExpose(mv->v,NULL,false);
}

static void MVLoadWordList(MetricsView *mv, int type) {
    int words_max = 1024*128;
    GTextInfo** words = WordlistLoadFileToGTextInfo( type, words_max );
    if ( !words ) {
	    GGadgetSetTitle8(mv->text,"");
	    return;
    }

    if ( words[0] ) {
	    GGadgetSetList(mv->text,words,true);
	    GGadgetSetTitle8(mv->text,(char *) (words[0]->text));
	    if ( type==-2 )
	        MVFigureGlyphNames(mv,_GGadgetGetTitle(mv->text)+1);
	    mv->word_index = 0;
    }
    GTextInfoArrayFree(words);
}

static int MV_TextChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	MetricsView *mv = GGadgetGetUserData(g);
	int pos = e->u.control.u.tf_changed.from_pulldown;
	if ( pos!=-1 ) {
	    int32 len;
	    GTextInfo **ti = GGadgetGetList(g,&len);
	    GTextInfo *cur = ti[pos];
	    int type = (intpt) cur->userdata;
	    if ( type < 0 )
		MVLoadWordList(mv,type);
	    else if ( cur->text!=NULL ) {
		mv->word_index = pos;
		if ( cur->text[0]==0x200b )	/* Zero width space, flag for glyph names */
		    MVFigureGlyphNames(mv,cur->text+1);
	    }
	}
	MVTextChanged(mv);
    }
return( true );
}

static int MV_ScriptLangChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *sstr = _GGadgetGetTitle(g);
	MetricsView *mv = GGadgetGetUserData(g);
	if ( e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	    GGadgetSetTitle8(g,mv->scriptlangs[e->u.control.u.tf_changed.from_pulldown].userdata );
	    sstr = _GGadgetGetTitle(g);
	} else {
	    if ( u_strlen(sstr)<4 || !isalpha(sstr[0]) || !isalnum(sstr[1]) /*|| !isalnum(sstr[2]) || !isalnum(sstr[3])*/ )
return( true );
	    if ( u_strlen(sstr)==4 )
		/* No language, we'll use default */;
	    else if ( u_strlen(sstr)!=10 || sstr[4]!='{' || sstr[9]!='}' ||
		    !isalpha(sstr[5]) || !isalpha(sstr[6]) || !isalpha(sstr[7])  )
return( true );
	}
	MVSetFeatures(mv);
	if ( mv->clen!=0 )/* if there are no chars, remetricking will set the script field to DFLT */
	    MVRemetric(mv);
	GDrawRequestExpose(mv->v,NULL,false);
    }
return( true );
}

static int MV_FeaturesChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MetricsView *mv = GGadgetGetUserData(g);
	MVRemetric(mv);
	GDrawRequestExpose(mv->v,NULL,false);
    }
return( true );
}

void MV_FriendlyFeatures(GGadget *g, int pos) {
    int32 len;
    GTextInfo **ti = GGadgetGetList(g,&len);

    if ( pos<0 || pos>=len )
	GGadgetEndPopup();
    else {
	const unichar_t *pt = ti[pos]->text;
	uint32 tag;
	int i;
	tag = (pt[0]<<24) | (pt[1]<<16) | (pt[2]<<8) | pt[3];
	LookupUIInit();
	for ( i=0; friendlies[i].friendlyname!=NULL; ++i )
	    if ( friendlies[i].tag==tag )
	break;
	if ( friendlies[i].friendlyname!=NULL )
	    GGadgetPreparePopup8(GGadgetGetWindow(g),friendlies[i].friendlyname);
    }
}

static int MV_SubtableChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MetricsView *mv = GGadgetGetUserData(g);
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	int i;
	KernPair *kp;
	struct lookup_subtable *sub;
	SplineFont *sf = mv->sf;

	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

	if ( ti[len-1]->selected ) {/* New lookup subtable */
	    struct subtable_data sd;
	    memset(&sd,0,sizeof(sd));
	    sd.flags = (mv->vertical ? sdf_verticalkern : sdf_horizontalkern ) |
		    sdf_kernpair | sdf_dontedit;
	    sub = SFNewLookupSubtableOfType(sf,gpos_pair,&sd,mv->layer);
	    if ( sub==NULL )
return( true );
	    mv->cur_subtable = sub;
	    MVSetSubtables(mv->sf);
	    MVSetFeatures(mv);		/* Is this needed? */
	} else if ( ti[len-2]->selected ) {	/* Idiots. They selected the line, can't have that */
	    MVSetSubtables(mv->sf);
	    sub = mv->cur_subtable;
	} else
	    mv->cur_subtable = GGadgetGetListItemSelected(mv->subtable_list)->userdata;

	for ( i=0; i<mv->glyphcnt; ++i ) {
	    if ( mv->perchar[i].selected )
	break;
	}
	kp = mv->glyphs[i].kp;
	if ( kp!=NULL )
	    kp->subtable = mv->cur_subtable;
    }
return( true );
}

#define MID_ZoomIn	2002
#define MID_ZoomOut	2003
#define MID_Next	2005
#define MID_Prev	2006
#define MID_Outline	2007
#define MID_ShowGrid	2008
#define MID_HideGrid	2009
#define MID_PartialGrid 2010
#define MID_HideGridWhenMoving	2011
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_AntiAlias	2014
#define MID_FindInFontView	2015
#define MID_Ligatures	2020
#define MID_KernPairs	2021
#define MID_AnchorPairs	2022
#define MID_Vertical	2023
#define MID_ReplaceChar	2024
#define MID_InsertCharB	2025
#define MID_InsertCharA	2026
#define MID_Layers	2027
#define MID_PointSize	2028
#define MID_Bigger	2029
#define MID_Smaller	2030
#define MID_SizeWindow	2031
#define MID_CharInfo	2201
#define MID_FindProblems 2216
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
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_SelAll	2106
#define MID_ClearSel	2105
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
#define MID_SetWidth	2601
#define MID_SetLBearing	2602
#define MID_SetRBearing	2603
#define MID_Thirds	2604
#define MID_VKernClass	2605
#define MID_VKernFromHKern	2606
#define MID_KernOnly	2607
#define MID_WidthOnly	2608
#define MID_BothKernWidth	2609
#define MID_SetBearings	2610
#define MID_Recent	2703
#define MID_SetVWidth	2705
#define MID_RemoveKerns	2707
#define MID_RemoveVKerns 2709

#define  MID_NextLineInWordList 2720
#define  MID_PrevLineInWordList 2721
#define MID_RenderUsingHinting	2722


#define MID_Warnings	3000

static void MVMenuOpen(GWindow gw, struct gmenuitem *mi, GEvent *g) {
    MetricsView *d = (MetricsView*)GDrawGetUserData(gw);
    FontView *fv = NULL;
    if (d) {
        fv = (FontView*)d->fv;
    }
    _FVMenuOpen(fv);
}

static void MVMenuClose(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    GDrawDestroyWindow(gw);
}

static void MVMenuOpenBitmap(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    EncMap *map;
    int i;

    if ( mv->sf->bitmaps==NULL )
return;
    for ( i=0; i<mv->glyphcnt; ++i )
        if ( mv->perchar[i].selected )
    break;
    map = mv->fv->b.map;
    if ( i!=mv->glyphcnt )
        BitmapViewCreatePick(map->backmap[mv->glyphs[i].sc->orig_pos],mv->fv);
}

static void MVMenuMergeKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MergeKernInfo(mv->sf,mv->fv->b.map);
}

static void MVMenuAddWordList(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e))
{
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVLoadWordList(mv,-1);
    GWidgetIndicateFocusGadget( mv->text );

    GEvent e;
    e.type = et_controlevent;
    e.u.control.subtype = et_textchanged;
    e.u.control.u.tf_changed.from_pulldown = 0;
    MV_TextChanged(mv->text, &e );
}


static void MVMenuOpenOutline(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->glyphcnt; ++i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt )
        CharViewCreate(mv->glyphs[i].sc, mv->fv, -1);
}

static void MVMenuSave(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuSave(mv->fv);
}

static void MVMenuSaveAs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuSaveAs(mv->fv);
}

static void MVMenuGenerate(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuGenerate(mv->fv, false);
}

static void MVMenuGenerateFamily(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuGenerate(mv->fv, gf_macfamily);
}

static void MVMenuGenerateTTC(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuGenerate(mv->fv, gf_ttc);
}

static void MVMenuPrint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    PrintFFDlg(NULL, NULL, mv);
}

static void MVUndo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_undo) )
        /* MVTextChanged(mv) */ ;
    else {
        for ( i=mv->glyphcnt-1; i>=0; --i )
            if ( mv->perchar[i].selected )
        break;
        if ( i==-1 )
return;
        if ( mv->glyphs[i].sc->layers[mv->layer].undoes!=NULL )
            SCDoUndo(mv->glyphs[i].sc, mv->layer);
    }
}

static void MVRedo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_redo) )
        /* MVTextChanged(mv) */ ;
    else {
        for ( i=mv->glyphcnt-1; i>=0; --i )
            if ( mv->perchar[i].selected )
        break;
        if ( i==-1 )
return;
        if ( mv->glyphs[i].sc->layers[mv->layer].redoes!=NULL )
            SCDoRedo(mv->glyphs[i].sc, mv->layer);
    }
}

static void MVClear(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    BDFFont *bdf;
    extern int onlycopydisplayed;

    if ( GGadgetActiveGadgetEditCmd(mv->gw, ec_clear) )
        /* MVTextChanged(mv) */;
    else {
        for ( i=mv->glyphcnt-1; i>=0; --i )
            if ( mv->perchar[i].selected )
        break;
        if ( i==-1 )
return;
        sc = mv->glyphs[i].sc;
        if ( sc->dependents!=NULL ) {
            int yes;
            char *buts[4];
            buts[1] = _("_Unlink");
            buts[0] = _("_Yes");
            buts[2] = _("_Cancel");
            buts[3] = NULL;
            yes = gwwv_ask(_("Bad Reference"), (const char **) buts, 1, 2, _("You are attempting to clear %.30s which is referred to by\nanother character. Are you sure you want to clear it?"), sc->name);
            if ( yes==2 )
return;
            if ( yes==1 )
                UnlinkThisReference(NULL, sc, mv->layer);
        }

        if ( onlycopydisplayed && mv->bdf==NULL ) {
            SCClearAll(sc, mv->layer);
        } else if ( onlycopydisplayed ) {
            BCClearAll(mv->bdf->glyphs[sc->orig_pos]);
        } else {
            SCClearAll(sc,mv->layer);
            for ( bdf=mv->sf->bitmaps; bdf!=NULL; bdf = bdf->next )
                BCClearAll(bdf->glyphs[sc->orig_pos]);
        }
    }
}

static void MVCut(GWindow gw, struct gmenuitem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw, ec_cut) )
        /* MVTextChanged(mv) */ ;
    else {
        for ( i=mv->glyphcnt-1; i>=0; --i )
            if ( mv->perchar[i].selected )
        break;
        if ( i==-1 )
return;
        MVCopyChar(&mv->fv->b,mv->bdf,mv->glyphs[i].sc,ct_fullcopy);
        MVClear(gw, mi, e); /* mi & e are actually not used */
    }
}

static void MVCopy(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw, ec_copy) )
        /* MVTextChanged(mv) */ ;
    else {
        for ( i=mv->glyphcnt-1; i>=0; --i )
            if ( mv->perchar[i].selected )
        break;
        if ( i==-1 )
return;
        MVCopyChar(&mv->fv->b, mv->bdf, mv->glyphs[i].sc, ct_fullcopy);
    }
}

static void MVMenuCopyRef(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    MVCopyChar(&mv->fv->b, mv->bdf, mv->glyphs[i].sc, ct_reference);
}

static void MVMenuCopyWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    SCCopyWidth(mv->glyphs[i].sc,
		   mi->mid==MID_CopyWidth?ut_width:
		   mi->mid==MID_CopyVWidth?ut_vwidth:
		   mi->mid==MID_CopyLBearing?ut_lbearing:
					 ut_rbearing);
}

static void MVMenuJoin(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, changed;
    extern float joinsnap;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    SCPreserveLayer(mv->glyphs[i].sc, mv->layer, false);
    mv->glyphs[i].sc->layers[mv->layer].splines =
        SplineSetJoin(mv->glyphs[i].sc->layers[mv->layer].splines, true, joinsnap, &changed);
    if ( changed )
        SCCharChangedUpdate(mv->glyphs[i].sc, mv->layer);
}

static void MVPaste(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw, ec_paste) )
        /*MVTextChanged(mv)*/ ;         /* Should get an event now */
    else {
        for ( i=mv->glyphcnt-1; i>=0; --i )
            if ( mv->perchar[i].selected )
        break;
        if ( i==-1 )
return;
        PasteIntoMV(&mv->fv->b, mv->bdf, mv->glyphs[i].sc, true);
    }
}

static void MVUnlinkRef(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    RefChar *rf, *next;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    sc = mv->glyphs[i].sc;
    SCPreserveLayer(sc, mv->layer,false);
    for ( rf=sc->layers[mv->layer].refs; rf!=NULL ; rf=next ) {
        next = rf->next;
        SCRefToSplines(sc, rf, mv->layer);
    }
    SCCharChangedUpdate(sc, mv->layer);
}

static void MVSelectAll(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    GGadgetActiveGadgetEditCmd(mv->gw, ec_selectall);
}

static void MVClearSelection(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    GWindowClearFocusGadgetOfWindow(mv->gw);
    for ( i=0; i<mv->glyphcnt; ++i )
        if ( mv->perchar[i].selected )
            MVDeselectChar(mv,i);
}

static void MVMenuFontInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    DelayEvent(FontMenuFontInfo, mv->fv);
}

static void MVMenuCharInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
        SCCharInfo(mv->glyphs[i].sc, mv->layer, mv->fv->b.map, -1);
}

static void MVMenuShowDependents(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
return;
    if ( mv->glyphs[i].sc->dependents==NULL )
return;
    SCRefBy(mv->glyphs[i].sc);
}

static void MVMenuFindProblems(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
        FindProblems(mv->fv, NULL, mv->glyphs[i].sc);
}

static void MVMenuBitmaps(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->glyphcnt; ++i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt )
        BitmapDlg(mv->fv, mv->glyphs[i].sc, mi->mid==MID_AvailBitmaps );
    else if ( mi->mid==MID_AvailBitmaps )
        BitmapDlg(mv->fv, NULL, true );
}

static int getorigin(void *d, BasePoint *base, int index) {
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

static void MVTransFunc(void *_sc, real transform[6], int UNUSED(otype),
        BVTFunc *UNUSED(bvts), enum fvtrans_flags flags ) {
    SplineChar *sc = _sc;
    FVTrans( (FontViewBase *)sc->parent->fv, sc, transform, NULL, flags);
}

static void MVMenuTransform(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
        TransformDlgCreate( mv->glyphs[i].sc, MVTransFunc, getorigin, true, cvt_none );
}

#ifdef FONTFORGE_CONFIG_TILEPATH
static void MVMenuTilePath(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
        SCTile(mv->glyphs[i].sc, mv->layer);
}
#endif

static void _MVMenuOverlap(MetricsView *mv, enum overlap_type ot) {
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	if ( !SCRoundToCluster(sc, mv->layer, false, 0.03, 0.12))
	    SCPreserveLayer(sc, mv->layer, false);
	MinimumDistancesFree(sc->md);
	sc->md = NULL;
	sc->layers[mv->layer].splines = SplineSetRemoveOverlap(sc, sc->layers[mv->layer].splines, ot);
	SCCharChangedUpdate(sc, mv->layer);
    }
}

static void MVMenuOverlap(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuOverlap(mv, mi->mid==MID_RmOverlap ? over_remove :
		       mi->mid==MID_Intersection ? over_intersect :
			   over_findinter);
}

static void MVMenuInline(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    OutlineDlg(NULL, NULL, mv, true);
}

static void MVMenuOutline(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    OutlineDlg(NULL, NULL, mv, false);
}

static void MVMenuShadow(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    ShadowDlg(NULL, NULL, mv, false);
}

static void MVMenuWireframe(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    ShadowDlg(NULL, NULL, mv, true);
}

static void MVSimplify( MetricsView *mv,int type ) {
    int i;
    static struct simplifyinfo smpls[] = {
	    { sf_normal, 0, 0, 0, 0, 0, 0 },
	    { sf_normal,.75,.05,0,-1, 0, 0 },
	    { sf_normal,.75,.05,0,-1, 0, 0 }};
    struct simplifyinfo *smpl = &smpls[type+1];

    if ( smpl->linelenmax==-1 ) {
	smpl->err = (mv->sf->ascent+mv->sf->descent)/1000.;
	smpl->linelenmax = (mv->sf->ascent+mv->sf->descent)/100.;
    }

    if ( type==1 ) {
	if ( !SimplifyDlg(mv->sf,smpl))
return;
	if ( smpl->set_as_default )
	    smpls[1] = *smpl;
    }

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	SCPreserveLayer(sc,mv->layer,false);
	sc->layers[mv->layer].splines = SplineCharSimplify(sc,sc->layers[mv->layer].splines,smpl);
	SCCharChangedUpdate(sc,mv->layer);
    }
}

static void MVMenuSimplify(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv, false);
}

static void MVMenuSimplifyMore(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv, true);
}

static void MVMenuCleanup(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv, -1);
}

static void MVMenuAddExtrema(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineFont *sf = mv->sf;
    int emsize = sf->ascent+sf->descent;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
        SplineChar *sc = mv->glyphs[i].sc;
        SCPreserveLayer(sc, mv->layer, false);
        SplineCharAddExtrema(sc, sc->layers[mv->layer].splines, ae_only_good, emsize);
        SCCharChangedUpdate(sc, mv->layer);
    }
}

static void MVMenuRound2Int(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
        SCPreserveLayer(mv->glyphs[i].sc, mv->layer, false);
        SCRound2Int( mv->glyphs[i].sc, mv->layer, 1.0);
    }
}

static void MVMenuAutotrace(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    GCursor ct;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
        ct = GDrawGetCursor(mv->gw);
        GDrawSetCursor(mv->gw, ct_watch);
        ff_progress_allow_events();
        SCAutoTrace(mv->glyphs[i].sc, mv->layer, e!=NULL && (e->u.mouse.state&ksm_shift));
        GDrawSetCursor(mv->gw, ct);
    }
}

static void MVMenuCorrectDir(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	int changed = false, refchanged=false;
	RefChar *ref;
	int asked=-1;

	for ( ref=sc->layers[mv->layer].refs; ref!=NULL; ref=ref->next ) {
	    if ( ref->transform[0]*ref->transform[3]<0 ||
		    (ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		if ( asked==-1 ) {
		    char *buts[4];
		    buts[0] = _("_Unlink");
		    buts[1] = _("_No");
		    buts[2] = _("_Cancel");
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
			SCPreserveLayer(sc,mv->layer,false);
		    }
		    SCRefToSplines(sc,ref,mv->layer);
		}
	    }
	}

	if ( !refchanged )
	    SCPreserveLayer(sc,mv->layer,false);
	sc->layers[mv->layer].splines = SplineSetsCorrect(sc->layers[mv->layer].splines,&changed);
	if ( changed || refchanged )
	    SCCharChangedUpdate(sc,mv->layer);
    }
}

static void _MVMenuBuildAccent(MetricsView *mv,int onlyaccents) {
    int i;
    extern int onlycopydisplayed;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	if ( SFIsSomethingBuildable(mv->sf,sc,mv->layer,onlyaccents) )
	    SCBuildComposit(mv->sf,sc,mv->layer,NULL,onlycopydisplayed);
    }
}

static void MVMenuBuildAccent(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuBuildAccent(mv, false);
}

static void MVMenuBuildComposite(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuBuildAccent(mv, true);
}

static void MVResetText(MetricsView *mv) {
    unichar_t *new, *pt;
    int i;

    new = malloc((mv->clen+1)*sizeof(unichar_t));
    for ( pt=new, i=0; i<mv->clen; ++i ) {
	if ( mv->chars[i]->unicodeenc==-1 )
	    *pt++ = MVFakeUnicodeOfSc(mv,mv->chars[i]);
	else
	    *pt++ = mv->chars[i]->unicodeenc;
    }
    *pt = '\0';
    GGadgetSetTitle(mv->text,new);
    free(new );
}

static void MVMenuLigatures(GWindow gw,struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowLigatures(mv->sf, NULL);
}

static void MVMenuKernPairs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowKernPairs(mv->sf, NULL, NULL, mv->layer);
}

static void MVMenuAnchorPairs(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowKernPairs(mv->sf, NULL, mi->ti.userdata, mv->layer);
}

static void _MVMenuScale( MetricsView *mv, int mid ) {

    if ( mid==MID_ZoomIn ) {
	if ( --mv->scale_index<0 ) mv->scale_index = 0;
    } else {
	if ( ++mv->scale_index >= sizeof(mv_scales)/sizeof(mv_scales[0]) )
	    mv->scale_index = sizeof(mv_scales)/sizeof(mv_scales[0])-1;
    }

    if ( mv->pixelsize_set_by_window ) {
	mv->pixelsize = mv_scales[mv->scale_index]*(mv->vheight - 2);
	if ( mv->bdf==NULL ) {
	    BDFFontFree(mv->show);
	    mv->show = SplineFontPieceMeal(mv->sf,mv->layer,mv->pixelsize,72,
					   MVGetSplineFontPieceMealFlags( mv ), NULL );
	} else
	    mv->pixelsize_set_by_window = false;
    }
    MVReKern(mv);
    MVSetVSb(mv);
}

static void MVMenuScale(GWindow gw,struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuScale(mv, mi->mid);
}

static void MVMenuInsertChar(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->sf;
    int i, j, pos = GotoChar(sf,mv->fv->b.map,NULL);

    if ( pos==-1 || pos>=mv->fv->b.map->enccount )
return;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt )	/* Something selected */
	/* Ok... */;
    else if ( mi->mid==MID_InsertCharA )
	i = mv->glyphcnt;
    else
	i = 0;
    if ( mi->mid==MID_InsertCharA ) {
	if ( i!=mv->glyphcnt )
	    ++i;
    } else {
	if ( i==mv->glyphcnt ) i = 0;
    }
    if ( i==mv->glyphcnt )
	i = mv->clen;
    else
	i = mv->glyphs[i].orig_index;		/* Index in the string of chars, not glyphs */

    if ( mv->clen+1>=mv->cmax ) {
	int oldmax=mv->cmax;
	mv->cmax = mv->clen+10;
	mv->chars = realloc(mv->chars,mv->cmax*sizeof(SplineChar *));
	memset(mv->chars+oldmax,'\0',(mv->cmax-oldmax)*sizeof(SplineChar *));
    }
    for ( j=mv->clen; j>i; --j )
	mv->chars[j] = mv->chars[j-1];
    mv->chars[i] = SFMakeChar(sf,mv->fv->b.map,pos);
    ++mv->clen;
    MVRemetric(mv);
    for ( j=0; j<mv->glyphcnt; ++j )
	if ( mv->glyphs[j].orig_index==i ) {
	    MVDoSelect(mv,j);
    break;
	}
    GDrawRequestExpose(mv->v,NULL,false);
    MVResetText(mv);
}

static void MVMenuChangeChar(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->sf;
    SplineChar *sc;
    EncMap *map = mv->fv->b.map;
    int i, pos, gid;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt ) {
	pos = -1;
	i = mv->glyphs[i].orig_index;
	sc = mv->chars[ i ];
	if ( mi->mid == MID_Next ) {
	    pos = map->backmap[sc->orig_pos]+1;
	} else if ( mi->mid==MID_Prev ) {
	    pos = map->backmap[sc->orig_pos]-1;
	} else if ( mi->mid==MID_NextDef ) {
	    for ( pos = map->backmap[sc->orig_pos]+1;
		    pos<map->enccount && ((gid=map->map[pos])==-1 || sf->glyphs[gid]==NULL); ++pos );
	    if ( pos>=map->enccount )
return;
	} else if ( mi->mid==MID_PrevDef ) {
	    for ( pos = map->backmap[sc->orig_pos]-1;
		    pos<map->enccount && ((gid=map->map[pos])==-1 || sf->glyphs[gid]==NULL); --pos );
	    if ( pos<0 )
return;
	} else if ( mi->mid==MID_ReplaceChar ) {
	    pos = GotoChar(sf,mv->fv->b.map,NULL);
	    if ( pos<0 || pos>=mv->fv->b.map->enccount)
return;
	}
	if ( pos>=0 && pos<map->enccount ) {
	    mv->chars[i] = SFMakeChar(sf,mv->fv->b.map,pos);
	    MVRemetric(mv);
	    MVResetText(mv);
	    GDrawRequestExpose(mv->v,NULL,false);
	}
    }
}

static void MVMenuFindInFontView(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->glyphcnt; ++i ) {
        if ( mv->perchar[i].selected ) {
            FVChangeChar(mv->fv, mv->fv->b.map->backmap[mv->glyphs[i].sc->orig_pos]);
            GDrawSetVisible(mv->fv->gw, true);
            GDrawRaise(mv->fv->gw);
    break;
        }
    }
}

static void MVMenuShowGrid(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    if ( mi->mid == MID_ShowGrid )
        mv->showgrid = mv_showgrid;
    else if ( mi->mid == MID_HideGrid )
        mv->showgrid = mv_hidegrid;
    else if ( mi->mid == MID_PartialGrid )
        mv->showgrid = mv_partialgrid;
    else if ( mi->mid == MID_HideGridWhenMoving )
        mv->showgrid = mv_hidemovinggrid;
    mvshowgrid = mv->showgrid;
    SavePrefs(true);
    GDrawRequestExpose(mv->v, NULL, false);
}

static void MVMenuAA(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    mv_antialias = mv->antialias = !mv->antialias;
    mv->bdf = NULL;
    BDFFontFree(mv->show);
    mv->show = SplineFontPieceMeal(mv->sf, mv->layer, mv->ptsize, mv->dpi,
                    MVGetSplineFontPieceMealFlags( mv ),
                    NULL);
    GDrawRequestExpose(mv->v,NULL,false);
}


static void MVMenuRenderUsingHinting(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    mv->usehinting = !mv->usehinting;
    mv->bdf = NULL;
    BDFFontFree(mv->show);
    mv->show = SplineFontPieceMeal(mv->sf, mv->layer, mv->ptsize, mv->dpi,
				   MVGetSplineFontPieceMealFlags( mv ),
				   NULL);
    GDrawRequestExpose(mv->v,NULL,false);
}

static void MVWindowTitle(char *buffer, int bufsize, MetricsView *mv) {

    snprintf(buffer,bufsize,
	    mv->type == mv_kernonly ?  _("Kerning Metrics For %.50s") :
	    mv->type == mv_widthonly ? _("Advance Width Metrics For %.50s") :
	                               _("Metrics For %.50s"),
		  mv->sf->fontname);
}

static void MVMenuWindowType(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    char buf[120];

    mv_type = mv->type = mi->mid==MID_KernOnly  ? mv_kernonly :
			 mi->mid==MID_WidthOnly ? mv_widthonly :
			 mv_kernwidth;
    MVWindowTitle(buf, sizeof(buf), mv);
    GDrawSetWindowTitles8(mv->gw, buf, buf);
    GDrawRequestExpose(mv->v, NULL, false);
    GDrawRequestExpose(mv->gw, NULL, false);
}

static void MVMenuVertical(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    if ( !mv->sf->hasvmetrics ) {
        if ( mv->vertical )
            MVToggleVertical(mv);
    } else
        MVToggleVertical(mv);
    GDrawRequestExpose(mv->gw, NULL, false);
    GDrawRequestExpose(mv->v, NULL, false);
}

static void MVMenuShowBitmap(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    BDFFont *bdf = mi->ti.userdata;

    if ( mv->bdf!=bdf ) {
        mv->pixelsize_set_by_window = bdf==NULL;
        if ( bdf!=NULL ) {
            mv->pixelsize = mv->ptsize = bdf->pixelsize;
            mv->dpi = 72;
        }
        MVChangeDisplayFont(mv, bdf);
        GDrawRequestExpose(mv->v, NULL, false);
    }
}

static void MVMoveInWordListByOffset( MetricsView *mv, int offset )
{
    if ( mv->word_index!=-1 ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(mv->text,&len);
	/* We subtract 3 because: There are two lines saying "load * list" */
	/*  and then a line with a rule on it which we don't want access to */
	if ( mv->word_index+offset >=0 && mv->word_index+offset<len-3 ) {
	    const unichar_t *tit;
	    mv->word_index += offset;
	    GGadgetSelectOneListItem(mv->text,mv->word_index);
	    tit = _GGadgetGetTitle(mv->text);
	    if ( tit!=NULL && tit[0]==0x200b )
		MVFigureGlyphNames(mv,tit+1);
	    else
		MVTextChanged(mv);
	    ti = NULL;
	}
    }
}

static void MVMenuNextLineInWordList(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVMoveInWordListByOffset( mv, 1 );
}
static void MVMenuPrevLineInWordList(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVMoveInWordListByOffset( mv, -1 );
}


#define CID_DPI		1002
#define CID_Size	1003

struct pxsz {
    MetricsView *mv;
    GWindow gw;
    int done;
};

static int PXSZ_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct pxsz *pxsz = GDrawGetUserData(GGadgetGetWindow(g));
	MetricsView *mv = pxsz->mv;
	int ptsize, dpi, err=0;

	ptsize = GetInt8( pxsz->gw, CID_Size, _("Point Size"), &err );
	dpi = GetInt8( pxsz->gw, CID_DPI, _("DPI"), &err );
	if ( err )
return(true);
	if ( ptsize<3 || ptsize>1500 || dpi<10 || dpi > 2000 ) {
	    ff_post_error(_("Number out of range"),_("Number out of range"));
return( true );
	}
	mv->pixelsize_set_by_window = false;
	mv->ptsize = ptsize;
	mv->dpi = dpi;
	mv->pixelsize = rint( (ptsize*dpi)/72.0 );
	if ( mv->bdf==NULL )
	    BDFFontFree(mv->show);
	mv->bdf = NULL;
	mv->show = SplineFontPieceMeal(mv->sf,mv->layer,mv->ptsize,mv->dpi,
				       MVGetSplineFontPieceMealFlags( mv ), NULL );

	MVReKern(mv);
	MVSetVSb(mv);
	pxsz->done = 2;
    }
return( true );
}

static int PXSZ_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct pxsz *pxsz = GDrawGetUserData(GGadgetGetWindow(g));
	pxsz->done = true;
    }
return( true );
}

static int pxsz_e_h(GWindow gw, GEvent *event) {
    struct pxsz *pxsz = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
return( false );
      case et_close:
	pxsz->done = true;
      break;
    }
return( true );
}

static void MVMenuPointSize(GWindow mgw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(mgw);
    struct pxsz pxsz;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7], *hvarray[5][3], *barray[8], boxes[3];
    GTextInfo label[7];
    int i,k;
    double iscale = mv->pixelsize_set_by_window ? 1.0 : mv_scales[mv->scale_index];
    char buffer[20], dbuffer[20];

    memset(&pxsz,0,sizeof(pxsz));
    pxsz.mv = mv;

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&boxes,0,sizeof(boxes));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Set Point Size");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    pxsz.gw = gw = GDrawCreateTopWindow(NULL,&pos,pxsz_e_h,&pxsz,&wattrs);

    k = i = 0;

    label[k].text = (unichar_t *) _("Point Size:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    /*gcd[k].gd.handle_controlevent = SS_ScriptChanged;*/
    gcd[k++].creator = GLabelCreate;
    hvarray[i][0] = &gcd[k-1];

    sprintf( buffer, "%d", (int) rint( mv->ptsize/iscale ));
    label[k].text = (unichar_t *) buffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Size;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[i][1] = &gcd[k-1]; hvarray[i++][2] = NULL;

    label[k].text = (unichar_t *) _("DPI:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    /*gcd[k].gd.handle_controlevent = SS_ScriptChanged;*/
    gcd[k++].creator = GLabelCreate;
    hvarray[i][0] = &gcd[k-1];

    sprintf( dbuffer, "%d", mv->dpi );
    label[k].text = (unichar_t *) dbuffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_DPI;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[i][1] = &gcd[k-1]; hvarray[i++][2] = NULL;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = PXSZ_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = PXSZ_Cancel;
    gcd[k++].creator = GButtonCreate;

    barray[0] = barray[2] = barray[3] = barray[4] = barray[6] = GCD_Glue; barray[7] = NULL;
    barray[1] = &gcd[k-2]; barray[5] = &gcd[k-1];
    hvarray[i][0] = &boxes[2]; hvarray[i][1] = GCD_ColSpan; hvarray[i++][2] = NULL;
    hvarray[i][0] = NULL;

    memset(boxes,0,sizeof(boxes));
    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);

    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
    while ( !pxsz.done )
        GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

static void MVMenuSizeWindow(GWindow mgw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(mgw);
    mv->pixelsize_set_by_window = true;
    mv->pixelsize = mv_scales[mv->scale_index]*(mv->vheight - 2);
    mv->dpi = 72;
    mv->ptsize = mv->pixelsize;
    if ( mv->bdf==NULL ) {
        BDFFontFree(mv->show);
        mv->show = SplineFontPieceMeal(
                        mv->sf, mv->layer, mv->pixelsize, 72,
			MVGetSplineFontPieceMealFlags( mv ),
                        NULL);
    }
    MVReKern(mv);
    MVSetVSb(mv);
}

static void MVMenuChangePointSize(GWindow mgw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(mgw);

    if ( mv->pixelsize_set_by_window )
return;
    if ( mi->mid==MID_Bigger )
        ++(mv->ptsize);
    else
        --(mv->ptsize);
    mv->pixelsize = rint( (mv->ptsize*mv->dpi)/72.0 );
    if ( mv->bdf==NULL )
        BDFFontFree(mv->show);
    mv->bdf = NULL;
    mv->show = SplineFontPieceMeal(mv->sf, mv->layer, mv->ptsize, mv->dpi,
				   MVGetSplineFontPieceMealFlags( mv ), NULL);

    MVReKern(mv);
    MVSetVSb(mv);
}

static void MVMenuChangeLayer(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    mv->layer = mi->mid;
    BDFFontFree(mv->show);
    mv->show = SplineFontPieceMeal(mv->sf, mv->layer, mv->ptsize, mv->dpi,
				   MVGetSplineFontPieceMealFlags( mv ), NULL);
    MVRemetric(mv);
    GDrawRequestExpose(mv->v,NULL,false);
}

static void MVMenuCenter(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    DBounds bb;
    real transform[6];
    SplineChar *sc;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt ) {
	sc = mv->glyphs[i].sc;
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0.0;
	SplineCharFindBounds(sc,&bb);
	if ( mi->mid==MID_Center )
	    transform[4] = (sc->width-(bb.maxx-bb.minx))/2 - bb.minx;
	else
	    transform[4] = (sc->width-(bb.maxx-bb.minx))/3 - bb.minx;
	if ( transform[4]!=0 )
	    FVTrans( (FontViewBase *)mv->fv,sc,transform,NULL, fvt_dontmovewidth| fvt_alllayers );
    }
}

static void MVMenuKernByClasses(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    ShowKernClasses(mv->sf, mv, mv->layer, false);
}

static void MVMenuVKernByClasses(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    ShowKernClasses(mv->sf, mv, mv->layer, true);
}

static void MVMenuVKernFromHKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    FVVKernFromHKern((FontViewBase *) mv->fv);
}

static void MVMenuKPCloseup(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineChar *sc1=NULL, *sc2=NULL;
    int i;

    for ( i=0; i<mv->glyphcnt; ++i ) {
        if ( mv->perchar[i].selected ) {
            sc1 = mv->glyphs[i].sc;
            if ( i+1<mv->glyphcnt )
                sc2 = mv->glyphs[i+1].sc;
    break;
        }
    }
    KernPairD(mv->sf,sc1,sc2,mv->layer,mv->vertical);
}

static GMenuItem2 wnmenu[] = {
    { { (unichar_t *) N_("New O_utline Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("New Outline Window|No Shortcut"), NULL, NULL, MVMenuOpenOutline, MID_OpenOutline },
    { { (unichar_t *) N_("New _Bitmap Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("New Bitmap Window|No Shortcut"), NULL, NULL, MVMenuOpenBitmap, MID_OpenBitmap },
    { { (unichar_t *) N_("New _Metrics Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("New Metrics Window|No Shortcut"), NULL, NULL, /* No function, never avail */NULL, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Warnings"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Warnings|No Shortcut"), NULL, NULL, _MenuWarnings, MID_Warnings },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    GMENUITEM2_EMPTY
};

static void MVWindowMenuBuild(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    struct gmenuitem *wmi;

    WindowMenuBuild(gw,mi,e);

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->glyphs[i].sc;

    for ( wmi = mi->sub; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case MID_OpenOutline:
	    wmi->ti.disabled = sc==NULL;
	  break;
	  case MID_OpenBitmap:
	    mi->ti.disabled = mv->sf->bitmaps==NULL || sc==NULL;
	  break;
	  case MID_Warnings:
	    wmi->ti.disabled = ErrorWindowExists();
	  break;
	}
    }
}

static GMenuItem2 dummyitem[] = {
    { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, NULL, NULL, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};
static GMenuItem2 fllist[] = {
    { { (unichar_t *) N_("Font|_New"), (GImage *) "filenew.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("New|No Shortcut"), NULL, NULL, MenuNew, 0 },
    { { (unichar_t *) N_("_Open"), (GImage *) "fileopen.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Open|No Shortcut"), NULL, NULL, MVMenuOpen, 0 },
    { { (unichar_t *) N_("Recen_t"), (GImage *) "filerecent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, NULL, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) N_("_Close"), (GImage *) "fileclose.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Close|No Shortcut"), NULL, NULL, MVMenuClose, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Save"), (GImage *) "filesave.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Save|No Shortcut"), NULL, NULL, MVMenuSave, 0 },
    { { (unichar_t *) N_("S_ave as..."), (GImage *) "filesaveas.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Save as...|No Shortcut"), NULL, NULL, MVMenuSaveAs, 0 },
    { { (unichar_t *) N_("_Generate Fonts..."), (GImage *) "filegenerate.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Generate Fonts...|No Shortcut"), NULL, NULL, MVMenuGenerate, 0 },
    { { (unichar_t *) N_("Generate Mac _Family..."), (GImage *) "filegeneratefamily.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Generate Mac Family...|No Shortcut"), NULL, NULL, MVMenuGenerateFamily, 0 },
    { { (unichar_t *) N_("Generate TTC..."), (GImage *) "filegeneratefamily.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Generate TTC...|No Shortcut"), NULL, NULL, MVMenuGenerateTTC, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Merge Feature Info..."), (GImage *) "filemergefeature.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Merge Kern Info...|No Shortcut"), NULL, NULL, MVMenuMergeKern, 0 },
    { { (unichar_t *) N_("Load _Word List..."), (GImage *) 0, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Load Word List...|No Shortcut"), NULL, NULL, MVMenuAddWordList, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Print..."), (GImage *) "fileprint.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Print...|No Shortcut"), NULL, NULL, MVMenuPrint, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Pr_eferences..."), (GImage *) "fileprefs.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Preferences...|No Shortcut"), NULL, NULL, MenuPrefs, 0 },
    { { (unichar_t *) N_("_X Resource Editor..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("X Resource Editor...|No Shortcut"), NULL, NULL, MenuXRes, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Quit"), (GImage *) "filequit.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'Q' }, H_("Quit|No Shortcut"), NULL, NULL, MenuExit, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 edlist[] = {
    { { (unichar_t *) N_("_Undo"), (GImage *) "editundo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Undo|No Shortcut"), NULL, NULL, MVUndo, MID_Undo },
    { { (unichar_t *) N_("_Redo"), (GImage *) "editredo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Redo|No Shortcut"), NULL, NULL, MVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Cu_t"), (GImage *) "editcut.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, H_("Cut|No Shortcut"), NULL, NULL, MVCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), (GImage *) "editcopy.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Copy|No Shortcut"), NULL, NULL, MVCopy, MID_Copy },
    { { (unichar_t *) N_("C_opy Reference"), (GImage *) "editcopyref.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Copy Reference|No Shortcut"), NULL, NULL, MVMenuCopyRef, MID_CopyRef },
    { { (unichar_t *) N_("Copy _Width"), (GImage *) "editcopywidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Copy Width|No Shortcut"), NULL, NULL, MVMenuCopyWidth, MID_CopyWidth },
    { { (unichar_t *) N_("Copy _VWidth"), (GImage *) "editcopyvwidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Copy VWidth|No Shortcut"), NULL, NULL, MVMenuCopyWidth, MID_CopyVWidth },
    { { (unichar_t *) N_("Co_py LBearing"), (GImage *) "editcopylbearing.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'p' }, H_("Copy LBearing|No Shortcut"), NULL, NULL, MVMenuCopyWidth, MID_CopyLBearing },
    { { (unichar_t *) N_("Copy RBearin_g"), (GImage *) "editcopyrbearing.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'g' }, H_("Copy RBearing|No Shortcut"), NULL, NULL, MVMenuCopyWidth, MID_CopyRBearing },
    { { (unichar_t *) N_("_Paste"), (GImage *) "editpaste.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Paste|No Shortcut"), NULL, NULL, MVPaste, MID_Paste },
    { { (unichar_t *) N_("C_lear"), (GImage *) "editclear.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Clear|No Shortcut"), NULL, NULL, MVClear, MID_Clear },
    { { (unichar_t *) N_("_Join"), (GImage *) "editjoin.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'J' }, H_("Join|No Shortcut"), NULL, NULL, MVMenuJoin, MID_Join },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Select _All"), (GImage *) "editselect.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Select All|No Shortcut"), NULL, NULL, MVSelectAll, MID_SelAll },
    { { (unichar_t *) N_("_Deselect All"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Clear Selection|Escape"), NULL, NULL, MVClearSelection, MID_ClearSel },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("U_nlink Reference"), (GImage *) "editunlink.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Unlink Reference|No Shortcut"), NULL, NULL, MVUnlinkRef, MID_UnlinkRef },
    GMENUITEM2_EMPTY
};

static GMenuItem2 smlist[] = {
    { { (unichar_t *) N_("_Simplify"), (GImage *) "elementsimplify.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Simplify|No Shortcut"), NULL, NULL, MVMenuSimplify, MID_Simplify },
    { { (unichar_t *) N_("Simplify More..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Simplify More...|No Shortcut"), NULL, NULL, MVMenuSimplifyMore, MID_SimplifyMore },
    { { (unichar_t *) N_("Clea_nup Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Cleanup Glyph|No Shortcut"), NULL, NULL, MVMenuCleanup, MID_CleanupGlyph },
    GMENUITEM2_EMPTY
};

static GMenuItem2 rmlist[] = {
    { { (unichar_t *) N_("_Remove Overlap"), (GImage *) "overlaprm.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Remove Overlap|No Shortcut"), NULL, NULL, MVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) N_("_Intersect"), (GImage *) "overlapintersection.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Intersect|No Shortcut"), NULL, NULL, MVMenuOverlap, MID_Intersection },
    { { (unichar_t *) N_("_Find Intersections"), (GImage *) "overlapfindinter.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Find Intersections|No Shortcut"), NULL, NULL, MVMenuOverlap, MID_FindInter },
    GMENUITEM2_EMPTY
};

static GMenuItem2 eflist[] = {
    { { (unichar_t *) N_("_Inline"), (GImage *) "stylesinline.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Inline|No Shortcut"), NULL, NULL, MVMenuInline, 0 },
    { { (unichar_t *) N_("_Outline"), (GImage *) "stylesoutline.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Outline|No Shortcut"), NULL, NULL, MVMenuOutline, 0 },
    { { (unichar_t *) N_("_Shadow"), (GImage *) "stylesshadow.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Shadow|No Shortcut"), NULL, NULL, MVMenuShadow, 0 },
    { { (unichar_t *) N_("_Wireframe"), (GImage *) "styleswireframe.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Wireframe|No Shortcut"), NULL, NULL, MVMenuWireframe, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 balist[] = {
    { { (unichar_t *) N_("_Build Accented Glyph"), (GImage *) "elementbuildaccent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Accented Glyph|No Shortcut"), NULL, NULL, MVMenuBuildAccent, MID_BuildAccent },
    { { (unichar_t *) N_("Build _Composite Glyph"), (GImage *) "elementbuildcomposite.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Composite Glyph|No Shortcut"), NULL, NULL, MVMenuBuildComposite, MID_BuildComposite },
    GMENUITEM2_EMPTY
};

static void balistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;

    for ( i=mv->glyphcnt-1; i>=0; --i )
        if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->glyphs[i].sc;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
        switch ( mi->mid ) {
          case MID_BuildAccent:
            mi->ti.disabled = sc==NULL || !SFIsSomethingBuildable(sc->parent, sc, mv->layer, true);
          break;
          case MID_BuildComposite:
            mi->ti.disabled = sc==NULL || !SFIsSomethingBuildable(sc->parent, sc, mv->layer, false);
          break;
        }
    }
}

static GMenuItem2 ellist[] = {
    { { (unichar_t *) N_("_Font Info..."), (GImage *) "elementfontinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Font Info...|No Shortcut"), NULL, NULL, MVMenuFontInfo, 0 },
    { { (unichar_t *) N_("Glyph _Info..."), (GImage *) "elementglyphinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Glyph Info...|No Shortcut"), NULL, NULL, MVMenuCharInfo, MID_CharInfo },
    { { (unichar_t *) N_("S_how Dependent"), (GImage *) "elementshowdep.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Show Dependent|No Shortcut"), NULL, NULL, MVMenuShowDependents, MID_ShowDependents },
    { { (unichar_t *) N_("Find Pr_oblems..."), (GImage *) "elementfindprobs.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Find Problems...|No Shortcut"), NULL, NULL, MVMenuFindProblems, MID_FindProblems },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Bitm_ap Strikes Available..."), (GImage *) "elementbitmapsavail.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Bitmap Strikes Available...|No Shortcut"), NULL, NULL, MVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) N_("Regenerate _Bitmap Glyphs..."), (GImage *) "elementregenbitmaps.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Regenerate Bitmap Glyphs...|No Shortcut"), NULL, NULL, MVMenuBitmaps, MID_RegenBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Transform..."), (GImage *) "elementtransform.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Transform...|No Shortcut"), NULL, NULL, MVMenuTransform, MID_Transform },
#ifdef FONTFORGE_CONFIG_TILEPATH
    { { (unichar_t *) N_("Tile _Path..."), (GImage *) "elementtilepath.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Tile Path...|No Shortcut"), NULL, NULL, MVMenuTilePath, MID_TilePath },
#endif
    { { (unichar_t *) N_("_Remove Overlap"), (GImage *) "rmoverlap.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Remove Overlap|No Shortcut"), rmlist, NULL, NULL, MID_RmOverlap },
    { { (unichar_t *) N_("_Simplify"), (GImage *) "elementsimplify.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Simplify|No Shortcut"), smlist, NULL, NULL, MID_Simplify },
    { { (unichar_t *) N_("Add E_xtrema"), (GImage *) "elementaddextrema.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'x' }, H_("Add Extrema|No Shortcut"), NULL, NULL, MVMenuAddExtrema, MID_AddExtrema },
    { { (unichar_t *) N_("To _Int"), (GImage *) "elementround.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("To Int|No Shortcut"), NULL, NULL, MVMenuRound2Int, MID_Round },
    { { (unichar_t *) N_("Effects"), (GImage *) "elementstyles.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Effects|No Shortcut"), eflist, NULL, NULL, MID_Effects },
    { { (unichar_t *) N_("Autot_race"), (GImage *) "elementautotrace.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Autotrace|No Shortcut"), NULL, NULL, MVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Correct Direction"), (GImage *) "elementcorrectdir.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Correct Direction|No Shortcut"), NULL, NULL, MVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("B_uild"), (GImage *) "elementbuildaccent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build|No Shortcut"), balist, balistcheck, NULL, MID_BuildAccent },
    GMENUITEM2_EMPTY
};

static GMenuItem2 dummyall[] = {
    { { (unichar_t *) N_("All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("All|No Shortcut"), NULL, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};

/* Builds up a menu containing all the anchor classes */
static void aplistbuild(GWindow base, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(base);

    GMenuItemArrayFree(mi->sub);
    mi->sub = NULL;

    _aplistbuild(mi, mv->sf, MVMenuAnchorPairs);
}

static GMenuItem2 cblist[] = {
    { { (unichar_t *) N_("_Kern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Kern Pairs|No Shortcut"), NULL, NULL, MVMenuKernPairs, MID_KernPairs },
    { { (unichar_t *) N_("_Anchored Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Anchored Pairs|No Shortcut"), dummyall, aplistbuild, MVMenuAnchorPairs, MID_AnchorPairs },
    { { (unichar_t *) N_("_Ligatures"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'L' }, H_("Ligatures|No Shortcut"), NULL, NULL, MVMenuLigatures, MID_Ligatures },
    GMENUITEM2_EMPTY
};

static void cblistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->sf;
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

static GMenuItem2 lylist[] = {
    { { (unichar_t *) N_("Layer|Foreground"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 1, 1, 1, 0, 0, 1, 1, 0, '\0' }, NULL, NULL, NULL, MVMenuChangeLayer, ly_fore },
    GMENUITEM2_EMPTY
};

static void lylistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->fv->b.sf;
    int ly;
    GMenuItem *sub;

    sub = calloc(sf->layer_cnt+1,sizeof(GMenuItem));
    for ( ly=ly_fore; ly<sf->layer_cnt; ++ly ) {
	sub[ly-1].ti.text = utf82u_copy(sf->layers[ly].name);
	sub[ly-1].ti.checkable = true;
	sub[ly-1].ti.checked = ly == mv->layer;
	sub[ly-1].invoke = MVMenuChangeLayer;
	sub[ly-1].mid = ly;
	sub[ly-1].ti.fg = sub[ly-1].ti.bg = COLOR_DEFAULT;
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = sub;
}

static GMenuItem2 gdlist[] = {
    { { (unichar_t *) N_("_Show"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Show Grid|No Shortcut"), NULL, NULL, MVMenuShowGrid, MID_ShowGrid },
    { { (unichar_t *) N_("_Partial"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Partial Grid|No Shortcut"), NULL, NULL, MVMenuShowGrid, MID_PartialGrid },
    { { (unichar_t *) N_("Hide when _Moving"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Hide Grid when Moving|No Shortcut"), NULL, NULL, MVMenuShowGrid, MID_HideGridWhenMoving },
    { { (unichar_t *) N_("_Hide"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Hide Grid|No Shortcut"), NULL, NULL, MVMenuShowGrid, MID_HideGrid },
    GMENUITEM2_EMPTY
};

static void gdlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_ShowGrid:
	    mi->ti.checked = mv->showgrid == mv_showgrid;
	  break;
	  case MID_HideGrid:
	    mi->ti.checked = mv->showgrid == mv_hidegrid;
	  break;
	  case MID_PartialGrid:
	    mi->ti.checked = mv->showgrid == mv_partialgrid;
	  break;
	  case MID_HideGridWhenMoving:
	    mi->ti.checked = mv->showgrid == mv_hidemovinggrid;
	  break;
	}
    }
}

static GMenuItem2 vwlist[] = {
    { { (unichar_t *) N_("Z_oom out"), (GImage *) "viewzoomout.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Zoom out|No Shortcut"), NULL, NULL, MVMenuScale, MID_ZoomOut },
    { { (unichar_t *) N_("Zoom _in"), (GImage *) "viewzoomin.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'i' }, H_("Zoom in|No Shortcut"), NULL, NULL, MVMenuScale, MID_ZoomIn },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Insert Glyph _After..."), (GImage *) "viewinsertafter.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Insert Glyph After...|No Shortcut"), NULL, NULL, MVMenuInsertChar, MID_InsertCharA },
    { { (unichar_t *) N_("Insert Glyph _Before..."), (GImage *) "viewinsertbefore.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Insert Glyph Before...|No Shortcut"), NULL, NULL, MVMenuInsertChar, MID_InsertCharB },
    { { (unichar_t *) N_("_Replace Glyph..."), (GImage *) "viewreplace.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Replace Glyph...|No Shortcut"), NULL, NULL, MVMenuChangeChar, MID_ReplaceChar },
    { { (unichar_t *) N_("_Next Glyph"), (GImage *) "viewnext.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("Next Glyph|No Shortcut"), NULL, NULL, MVMenuChangeChar, MID_Next },
    { { (unichar_t *) N_("_Prev Glyph"), (GImage *) "viewprev.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Prev Glyph|No Shortcut"), NULL, NULL, MVMenuChangeChar, MID_Prev },
    { { (unichar_t *) N_("Next _Defined Glyph"), (GImage *) "viewnextdef.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Next Defined Glyph|No Shortcut"), NULL, NULL, MVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) N_("Prev Defined Gl_yph"), (GImage *) "viewprevdef.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Prev Defined Glyph|No Shortcut"), NULL, NULL, MVMenuChangeChar, MID_PrevDef },
    { { (unichar_t *) N_("Find In Font _View"), (GImage *) "viewfindinfont.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Find In Font View|No Shortcut"), NULL, NULL, MVMenuFindInFontView, MID_FindInFontView },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Layers"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Layers|No Shortcut"), lylist, lylistcheck, NULL, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Com_binations"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'b' }, H_("Combinations|No Shortcut"), cblist, cblistcheck, NULL, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Show _Grid"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Show Grid|No Shortcut"), gdlist, gdlistcheck, MVMenuShowGrid, MID_ShowGrid },
    { { (unichar_t *) N_("_Anti Alias"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'A' }, H_("Anti Alias|No Shortcut"), NULL, NULL, MVMenuAA, MID_AntiAlias },
    { { (unichar_t *) N_("Render using Hinting"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'A' }, H_("Render using Hinting|No Shortcut"), NULL, NULL, MVMenuRenderUsingHinting, MID_RenderUsingHinting },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Vertical"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, '\0' }, H_("Vertical|No Shortcut"), NULL, NULL, MVMenuVertical, MID_Vertical },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Size set from _Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'O' }, H_("Size set from Window|No Shortcut"), NULL, NULL, MVMenuSizeWindow, MID_SizeWindow },
    { { (unichar_t *) N_("Set Point _Size"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'O' }, H_("Set Point Size|No Shortcut"), NULL, NULL, MVMenuPointSize, MID_PointSize },
    { { (unichar_t *) N_("_Bigger Point Size"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Bigger Point Size|No Shortcut"), NULL, NULL, MVMenuChangePointSize, MID_Bigger },
    { { (unichar_t *) N_("_Smaller Point Size"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Smaller Point Size|No Shortcut"), NULL, NULL, MVMenuChangePointSize, MID_Smaller },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Next _Line in Word List"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'O' }, H_("Next Line in Word List|No Shortcut"), NULL, NULL, MVMenuNextLineInWordList, MID_NextLineInWordList },
    { { (unichar_t *) N_("Previous Line in _Word List"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'O' }, H_("Previous Line in Word List|No Shortcut"), NULL, NULL, MVMenuPrevLineInWordList, MID_PrevLineInWordList },

    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'O' }, H_("Outline|No Shortcut"), NULL, NULL, MVMenuShowBitmap, MID_Outline },
    GMENUITEM2_EMPTY,
    /* Some extra room to show bitmaps */
    GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY,
    GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY,
    GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY, GMENUITEM2_EMPTY,
    GMENUITEM2_EMPTY
};

static void MVMenuContextualHelp(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("metricsview.html");
}

static GMenuItem2 tylist[] = {
    { { (unichar_t *) N_("_Kerning only"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Kerning only|No Shortcut"), NULL, NULL, MVMenuWindowType, MID_KernOnly },
    { { (unichar_t *) N_("_Advance Width only"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Advance Width Only|No Shortcut"), NULL, NULL, MVMenuWindowType, MID_WidthOnly },
    { { (unichar_t *) N_("_Both"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Both|No Shortcut"), NULL, NULL, MVMenuWindowType, MID_BothKernWidth },
    GMENUITEM2_EMPTY
};

static void tylistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_KernOnly:
	    mi->ti.checked = mv->type == mv_kernonly;
	  break;
	  case MID_WidthOnly:
	    mi->ti.checked = mv->type == mv_widthonly;
	  break;
	  case MID_BothKernWidth:
	    mi->ti.checked = mv->type == mv_kernwidth;
	  break;
	}
    }
}



static void MVMenuSetWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    if ( mi->mid == MID_SetVWidth && !mv->sf->hasvmetrics )
return;
    SplineChar* sc = getSelectedChar(mv);
    printf("MVMenuSetWidth() sc:%p\n",sc);
    if(!sc)
 	return;

    GenericVSetWidth(mv->fv,sc,
		     mi->mid==MID_SetWidth?wt_width:
		     mi->mid==MID_SetLBearing?wt_lbearing:
		     mi->mid==MID_SetRBearing?wt_rbearing:
		     mi->mid==MID_SetBearings?wt_bearings:
		     wt_vwidth);
}

static void MVMenuRemoveKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineChar* sc = getSelectedChar(mv);
    if(!sc)
 	return;
    SCRemoveKern(sc);
}
static void MVMenuRemoveVKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineChar* sc = getSelectedChar(mv);
    if(!sc)
 	return;
    SCRemoveVKern(sc);
}

static GMenuItem2 mtlist[] = {
    { { (unichar_t *) N_("_Center in Width"), (GImage *) "metricscenter.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Center in Width|No Shortcut"), NULL, NULL, MVMenuCenter, MID_Center },
    { { (unichar_t *) N_("_Thirds in Width"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Thirds in Width|No Shortcut"), NULL, NULL, MVMenuCenter, MID_Thirds },
    { { (unichar_t *) N_("Set _Width..."), (GImage *) "metricssetwidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Set Width...|No Shortcut"), NULL, NULL, MVMenuSetWidth, MID_SetWidth },
    { { (unichar_t *) N_("Set _LBearing..."), (GImage *) "metricssetlbearing.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'L' }, H_("Set LBearing...|No Shortcut"), NULL, NULL, MVMenuSetWidth, MID_SetLBearing },
    { { (unichar_t *) N_("Set _RBearing..."), (GImage *) "metricssetrbearing.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Set RBearing...|No Shortcut"), NULL, NULL, MVMenuSetWidth, MID_SetRBearing },
    { { (unichar_t *) N_("Set Both Bearings..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Set Both Bearings...|No Shortcut"), NULL, NULL, MVMenuSetWidth, MID_SetBearings },
    GMENUITEM2_LINE,
    { { (unichar_t *) N_("Set _Vertical Advance..."), (GImage *) "metricssetvwidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Set Vertical Advance...|No Shortcut"), NULL, NULL, MVMenuSetWidth, MID_SetVWidth },
    GMENUITEM2_LINE,
    { { (unichar_t *) N_("_Window Type"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Window Type|No Shortcut"), tylist, tylistcheck, NULL, 0 },
    GMENUITEM2_LINE,
    { { (unichar_t *) N_("Ker_n By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Kern By Classes...|No Shortcut"), NULL, NULL, MVMenuKernByClasses, 0 },
    { { (unichar_t *) N_("VKern By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("VKern By Classes...|No Shortcut"), NULL, NULL, MVMenuVKernByClasses, MID_VKernClass },
    { { (unichar_t *) N_("VKern From HKern"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("VKern From HKern|No Shortcut"), NULL, NULL, MVMenuVKernFromHKern, MID_VKernFromHKern },
    { { (unichar_t *) N_("Remove Kern _Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Remove Kern Pairs|No Shortcut"), NULL, NULL, MVMenuRemoveKern, MID_RemoveKerns },
    { { (unichar_t *) N_("Remove VKern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Remove VKern Pairs|No Shortcut"), NULL, NULL, MVMenuRemoveVKern, MID_RemoveVKerns },
    { { (unichar_t *) N_("Kern Pair Closeup..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Kern Pair Closeup...|No Shortcut"), NULL, NULL, MVMenuKPCloseup, 0 },
    GMENUITEM2_EMPTY
};

static void fllistcheck(GWindow UNUSED(gw), struct gmenuitem *mi, GEvent *UNUSED(e)) {
    /*MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);*/

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static void edlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
	i = -1;
    else
	for ( i=mv->glyphcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Cut: case MID_Copy:
	  break;
	  case MID_Join:
	  case MID_CopyRef: case MID_CopyWidth:
	  case MID_CopyLBearing: case MID_CopyRBearing:
	  case MID_Clear:
	    mi->ti.disabled = i==-1;
	  break;
	  case MID_CopyVWidth:
	    mi->ti.disabled = i==-1 || !mv->sf->hasvmetrics;
	  break;
	  case MID_UnlinkRef:
	    mi->ti.disabled = i==-1 || mv->glyphs[i].sc->layers[mv->layer].refs==NULL;
	  break;
	  case MID_Paste:
	  break;
	}
    }
}

static void ellistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, anybuildable;
    SplineChar *sc;
    int order2 = mv->sf->layers[mv->layer].order2;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->glyphs[i].sc;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_RegenBitmaps:
	    mi->ti.disabled = mv->sf->bitmaps==NULL;
	  break;
	  case MID_CharInfo:
	    mi->ti.disabled = sc==NULL /*|| mv->fv->b.cidmaster!=NULL*/;
	  break;
	  case MID_ShowDependents:
	    mi->ti.disabled = sc==NULL || sc->dependents == NULL;
	  break;
	  case MID_FindProblems:
	  case MID_Transform:
	    mi->ti.disabled = sc==NULL;
	  break;
	  case MID_Effects:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps || order2;
	  break;
	  case MID_RmOverlap: case MID_Stroke:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps;
	  break;
	  case MID_AddExtrema:
	  case MID_Round: case MID_Correct:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps;
	  break;
#ifdef FONTFORGE_CONFIG_TILEPATH
	  case MID_TilePath:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps || ClipBoardToSplineSet()==NULL || order2;
	  break;
#endif
	  case MID_Simplify:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps;
	  break;
	  case MID_BuildAccent:
	    anybuildable = false;
	    if ( sc!=NULL && SFIsSomethingBuildable(mv->sf,sc,mv->layer,false) )
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

static void vwlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, j, base, aselection;
    BDFFont *bdf;
    char buffer[60];

    aselection = false;
    for ( j=0; j<mv->glyphcnt; ++j )
	if ( mv->perchar[j].selected ) {
	    aselection = true;
    break;
	}

    for ( i=0; vwlist[i].mid!=MID_Outline; ++i )
	switch ( vwlist[i].mid ) {
	  case MID_ZoomIn:
	    vwlist[i].ti.disabled = mv->scale_index==0;
	  break;
	  case MID_ZoomOut:
	    vwlist[i].ti.disabled = mv->scale_index>=sizeof(mv_scales)/sizeof(mv_scales[0])-1;
	  break;
	  case MID_AntiAlias:
	    vwlist[i].ti.checked = mv->antialias;
	    vwlist[i].ti.disabled = mv->bdf!=NULL;
	  break;
	  case MID_RenderUsingHinting:
	    vwlist[i].ti.checked = mv->usehinting;
	    vwlist[i].ti.disabled = mv->bdf!=NULL;
	  break;
	  case MID_SizeWindow:
	    vwlist[i].ti.disabled = mv->pixelsize_set_by_window;
	    vwlist[i].ti.checked = mv->pixelsize_set_by_window;
	  break;
	  case MID_Bigger:
	  case MID_Smaller:
	    vwlist[i].ti.disabled = mv->pixelsize_set_by_window;
	  break;
	  case MID_ReplaceChar:
	  case MID_FindInFontView:
	  case MID_Next:
	  case MID_Prev:
	  case MID_NextDef:
	  case MID_PrevDef:
	    vwlist[i].ti.disabled = !aselection;
	  break;
	  case MID_Vertical:
	    vwlist[i].ti.checked = mv->vertical;
	    vwlist[i].ti.disabled = !mv->sf->hasvmetrics;
	  break;
	  case MID_Layers:
	    vwlist[i].ti.disabled = mv->sf->layer_cnt<=2 || mv->sf->multilayer;
	  break;
	}
    vwlist[i].ti.checked = mv->bdf==NULL;
    base = i+1;
    for ( i=base; vwlist[i].ti.text!=NULL || vwlist[i].ti.line; ++i ) {
	free( vwlist[i].ti.text);
	vwlist[i].ti.text = NULL;
    }

    if ( mv->sf->bitmaps!=NULL ) {
	for ( bdf = mv->sf->bitmaps, i=base;
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
    mi->sub = GMenuItem2ArrayCopy(vwlist,NULL);
}

static void mtlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineChar* sc = getSelectedChar(mv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
        switch ( mi->mid ) {
	  case MID_VKernClass:
	  case MID_VKernFromHKern:
	  case MID_SetVWidth:
	    mi->ti.disabled = !mv->sf->hasvmetrics;
	  break;
	case MID_RemoveKerns:
	    mi->ti.disabled = sc ? sc->kerns==NULL : 1;
	  break;
	case MID_RemoveVKerns:
	    mi->ti.disabled = sc ? sc->vkerns==NULL : 1;
	  break;

	}
    }
}

static GMenuItem2 mblist[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("File|No Shortcut"), fllist, fllistcheck, NULL, 0  },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'E' }, H_("Edit|No Shortcut"), edlist, edlistcheck, NULL, 0  },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Element|No Shortcut"), ellist, ellistcheck, NULL, 0  },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("View|No Shortcut"), vwlist, vwlistcheck, NULL, 0  },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Metrics|No Shortcut"), mtlist, mtlistcheck, NULL, 0  },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Window|No Shortcut"), wnmenu, MVWindowMenuBuild, NULL, 0 },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Help|No Shortcut"), helplist, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};

static void MVResize(MetricsView *mv) {
    GRect pos, wsize;
    int i;
    int size;

    GDrawGetSize(mv->gw,&wsize);
    if ( wsize.height < mv->topend+20 + mv->height-mv->displayend ||
	    wsize.width < 30 ) {
	int width= wsize.width < 30 ? 30 : wsize.width;
	int height;

	if ( wsize.height < mv->topend+20 + mv->height-mv->displayend )
	    height = mv->topend+20 + mv->height-mv->displayend;
	else
	    height = wsize.height;
	GDrawResize(mv->gw,width,height);
return;
    }

    mv->width = wsize.width;
    mv->displayend = wsize.height - (mv->height-mv->displayend);
    mv->height = wsize.height;

    mv_width = wsize.width; mv_height = wsize.height;
    SavePrefs(true);

    pos.width = wsize.width;
    pos.height = mv->sbh;
    pos.y = wsize.height - pos.height; pos.x = 0;
    GGadgetResize(mv->hsb,pos.width,pos.height);
    GGadgetMove(mv->hsb,pos.x,pos.y);

    mv->dwidth = mv->width - mv->sbh;
    GGadgetResize(mv->vsb,mv->sbh,mv->displayend-mv->topend);
    GGadgetMove(mv->vsb,wsize.width-mv->sbh,mv->topend);

    GGadgetResize(mv->features,mv->xstart,mv->displayend - mv->topend);

    size = (mv->displayend - mv->topend - 4);
    if ( mv->dwidth-20<size )
	size = mv->dwidth-20;
    if ( mv->pixelsize_set_by_window ) {
	mv->ptsize = mv->pixelsize = mv_scales[mv->scale_index]*size;
	mv->dpi = 72;
	if ( mv->bdf==NULL ) {
	    BDFFontFree(mv->show);
	    mv->show = SplineFontPieceMeal(mv->sf,mv->layer,mv->ptsize,mv->dpi,
					   MVGetSplineFontPieceMealFlags( mv ), NULL );
	}
    }

    for ( i=0; i<mv->max; ++i ) if ( mv->perchar[i].width!=NULL ) {
	GGadgetMove(mv->perchar[i].name,mv->perchar[i].mx,mv->displayend+2);
	GGadgetMove(mv->perchar[i].width,mv->perchar[i].mx,mv->displayend+2+mv->fh+4);
	GGadgetMove(mv->perchar[i].lbearing,mv->perchar[i].mx,mv->displayend+2+2*(mv->fh+4));
	GGadgetMove(mv->perchar[i].rbearing,mv->perchar[i].mx,mv->displayend+2+3*(mv->fh+4));
	if ( mv->perchar[i].kern!=NULL )
	    GGadgetMove(mv->perchar[i].kern,mv->perchar[i].mx-mv->perchar[i].mwidth/2,mv->displayend+2+4*(mv->fh+4));
    }
    GGadgetMove(mv->namelab,2,mv->displayend+2);
    GGadgetMove(mv->widthlab,2,mv->displayend+2+mv->fh+4);
    GGadgetMove(mv->lbearinglab,2,mv->displayend+2+2*(mv->fh+4));
    GGadgetMove(mv->rbearinglab,2,mv->displayend+2+3*(mv->fh+4));
    GGadgetMove(mv->kernlab,2,mv->displayend+2+4*(mv->fh+4));

    {
      int newwidth = mv->width;
      GRect scriptselector_size;
      GRect charselector_size;
      GRect charselectorNext_size;
      GRect charselectorPrev_size;
      GRect subtable_list_size;
      GGadgetGetSize(mv->script, &scriptselector_size);
      GGadgetGetSize(mv->text, &charselector_size);
      GGadgetGetSize(mv->textPrev, &charselectorPrev_size);
      GGadgetGetSize(mv->textNext, &charselectorNext_size);
      GGadgetGetSize(mv->subtable_list, &subtable_list_size);
      int new_charselector_width = newwidth - charselector_size.x -       charselectorNext_size.width - 2 - charselectorPrev_size.width - 2 - subtable_list_size.width - 10 - 10;
      if (new_charselector_width < GDrawPointsToPixels(mv->gw,100))
        new_charselector_width = GDrawPointsToPixels(mv->gw,100);
      int new_charselectorPrev_x = charselector_size.x + new_charselector_width + 4;
      int new_charselectorNext_x = new_charselectorPrev_x + charselectorPrev_size.width + 4;
      int new_subtableselector_x = new_charselectorNext_x + charselectorNext_size.width + 10;

      GGadgetResize(mv->text, new_charselector_width, charselector_size.height);
      GGadgetMove(mv->textPrev, new_charselectorPrev_x, charselectorPrev_size.y);
      GGadgetMove(mv->textNext, new_charselectorNext_x, charselectorNext_size.y);
      GGadgetMove(mv->subtable_list, new_subtableselector_x, subtable_list_size.y);
    }

    mv->vwidth = mv->dwidth-mv->xstart;
    mv->vheight = mv->displayend-mv->topend-2;
    GDrawResize(mv->v,mv->vwidth, mv->vheight);
    MVRemetric(mv);
    GDrawRequestExpose(mv->gw,NULL,true);
    GDrawRequestExpose(mv->v,NULL,true);
}

static void MVChar(MetricsView *mv,GEvent *event)
{
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
    if ( event->u.chr.keysym == GK_Up || event->u.chr.keysym==GK_KP_Up
	 || event->u.chr.keysym == GK_Down || event->u.chr.keysym==GK_KP_Down
	 || event->u.chr.keysym == GK_Left || event->u.chr.keysym==GK_KP_Left
	 || event->u.chr.keysym == GK_Right || event->u.chr.keysym==GK_KP_Right ) {
	if( event->u.chr.state&ksm_meta ) {
	    int dir = ( event->u.chr.keysym == GK_Up || event->u.chr.keysym==GK_KP_Up ) ? -1
		: ( ( event->u.chr.keysym == GK_Down || event->u.chr.keysym==GK_KP_Down ) ? 1 : 0);
	    GGadget *active = GWindowGetFocusGadgetOfWindow(mv->gw);
	    GGadget *toSelect = 0;

	    if( active ) {
		int i=0, j=0;
		for ( i=0; i<mv->glyphcnt; ++i ) {
		    // remember here that j=0 is NULL because updownkparray is NULL terminated
		    // at both ends.
		    for ( j=1; j<10 && mv->perchar[i].updownkparray[j]; ++j ) {
			if ( active == mv->perchar[i].updownkparray[j] ) {
			    if( dir != 0 ) {
				toSelect =  mv->perchar[i].updownkparray[j+dir];
			    } else {
				int newidx = i;
				if( event->u.chr.keysym == GK_Left || event->u.chr.keysym==GK_KP_Left )
				    newidx--;
				if( event->u.chr.keysym == GK_Right || event->u.chr.keysym==GK_KP_Right )
				    newidx++;
				if( newidx < 0 || newidx >= mv->glyphcnt )
				    return;
				toSelect =  mv->perchar[newidx].updownkparray[j];
			    }
			}
		    }
		}
	    }
	    if( toSelect ) {
		GWidgetIndicateFocusGadget(toSelect);
	    }
	    return;
	}
    }
    if ( event->u.chr.keysym == GK_Up || event->u.chr.keysym==GK_KP_Up ||
	    event->u.chr.keysym == GK_Down || event->u.chr.keysym==GK_KP_Down ) {
	    GGadget *active = GWindowGetFocusGadgetOfWindow(mv->gw);
	    if(!active)
		return;

	    // MIQ: We do not want to increment and decrement the integer
	    //      value of the kerning word on up/down now, instead we
	    //      should always move up/down in the list of kerning words.
	    if( active != mv->text )
	    {
		unichar_t *end;
		double val = u_strtod(_GGadgetGetTitle(active),&end);
		if (isValidInt(end)) {
		    int dir = ( event->u.chr.keysym == GK_Up || event->u.chr.keysym==GK_KP_Up ) ? 1 : -1;
		    if( event->u.chr.state&ksm_control && event->u.chr.state&ksm_shift ) {
			dir *= pref_mv_control_shift_and_arrow_skip;
		    }
		    else if( event->u.chr.state&ksm_shift ) {
			dir *= pref_mv_shift_and_arrow_skip;
		    }
		    val += dir;
		    char buf[100];
		    snprintf(buf,99,"%.0f",val);
		    GGadgetSetTitle8(active, buf);

		    event->u.control.u.tf_changed.from_pulldown=-1;
		    event->type=et_controlevent;
		    event->u.control.subtype = et_textchanged;
		    GGadgetDispatchEvent(active,event);

		    if( haveClassBasedKerningInView(mv) )
		    {
			MVRemetric(mv);
			GDrawRequestExpose(mv->v,NULL,false);
		    }
		}
	    }
    }
    if ( event->u.chr.keysym == GK_Up || event->u.chr.keysym==GK_KP_Up ||
	 event->u.chr.keysym == GK_Down || event->u.chr.keysym==GK_KP_Down )
    {
	int dir = ( event->u.chr.keysym == GK_Up || event->u.chr.keysym==GK_KP_Up ) ? -1 : 1;
	MVMoveInWordListByOffset( mv, dir );
    }
}

static int hitsbit(BDFChar *bc, int x, int y) {
    if ( bc->byte_data )
return( bc->bitmap[y*bc->bytes_per_line+x] );
    else
return( bc->bitmap[y*bc->bytes_per_line+(x>>3)]&(1<<(7-(x&7))) );
}

static void _MVSubVMouse(MetricsView *mv,GEvent *event) {
    int i, x, y, j, within, xbase;
    SplineChar *sc;
    int diff;
    int onwidth, onkern;
    SplineFont *sf = mv->sf;
    double iscale = mv->pixelsize_set_by_window ? 1.0 : mv_scales[mv->scale_index];
    double scale = iscale*mv->pixelsize/(double) (sf->ascent+sf->descent);
    int as = rint(sf->ascent*scale);

    xbase = mv->vwidth/2;
    within = -1;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	y = mv->perchar[i].dy + mv->perchar[i].yoff;
	x = xbase - mv->pixelsize*iscale/2 - mv->perchar[i].xoff;
	if ( mv->bdf==NULL ) {
	    BDFChar *bdfc = BDFPieceMealCheck(mv->show,mv->glyphs[i].sc->orig_pos);
	    if ( event->u.mouse.x >= x+bdfc->xmin &&
		event->u.mouse.x <= x+bdfc->xmax &&
		event->u.mouse.y <= (y+as)-bdfc->ymin &&
		event->u.mouse.y >= (y+as)-bdfc->ymax &&
		hitsbit(bdfc,rint(iscale*(event->u.mouse.x-x-bdfc->xmin)),
			rint(iscale*(event->u.mouse.y-(y+as-bdfc->ymax)))) )
    break;
	}
	y += -mv->perchar[i].yoff;
	if ( event->u.mouse.y >= y && event->u.mouse.y < y+mv->perchar[i].dheight+ mv->perchar[i].kernafter )
	    within = i;
    }
    if ( i==mv->glyphcnt )
	sc = NULL;
    else
	sc = mv->glyphs[i].sc;

    diff = event->u.mouse.y-mv->pressed_y;
    onwidth = onkern = false;
    if ( sc==NULL ) {
	if ( mv->type == mv_kernonly ) {
	    if ( within!=-1 && within+1<mv->glyphcnt &&
		    event->u.mouse.y>mv->perchar[within+1].dy-3 ) {
		onkern = true;			/* subsequent char */
		++within;
	    } else if ( within>0 &&
		    event->u.mouse.y<mv->perchar[within].dy+3 )
		onkern = true;
	} else if ( mv->type == mv_widthonly ) {
	    if ( within!=-1 && within+1<mv->glyphcnt &&
		    event->u.mouse.y>mv->perchar[within+1].dy-3 )
		onwidth = true;			/* subsequent char */
	    else if ( within>=0 &&
		    event->u.mouse.y>mv->perchar[within].dy+mv->perchar[within].dheight+mv->perchar[within].kernafter-3 ) {
		onwidth = true;
	    }
	} else {
	    if ( within>0 && mv->perchar[within-1].selected &&
		    event->u.mouse.y<mv->perchar[within].dy+3 )
		onwidth = true;		/* previous char */
	    else if ( within!=-1 && within+1<mv->glyphcnt &&
		    mv->perchar[within+1].selected &&
		    event->u.mouse.y>mv->perchar[within+1].dy-3 ) {
		onkern = true;			/* subsequent char */
		++within;
	    } else if ( within>0 && mv->perchar[within].selected &&
		    event->u.mouse.y<mv->perchar[within].dy+3 )
		onkern = true;
	    else if ( within>=0 &&
		    event->u.mouse.y>mv->perchar[within].dy+mv->perchar[within].dheight+mv->perchar[within].kernafter-3 ) {
		onwidth = true;
	    }
	}
    }

    if ( event->type != et_mousemove || !mv->pressed ) {
	int ct = -1;
	if ( mv->bdf!=NULL ||
		( mv->type==mv_kernonly && !onkern ) ||
		( mv->type==mv_widthonly && !onwidth )) {
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
	    sc = mv->glyphs[within].sc;
	if ( sc!=NULL )
	    SCPreparePopup(mv->gw,sc,mv->fv->b.map->remap,mv->fv->b.map->backmap[sc->orig_pos],sc->unicodeenc);
/* Don't allow any editing when displaying a bitmap font */
    } else if ( event->type == et_mousedown && mv->bdf==NULL ) {
	CVPaletteDeactivate();
	if ( sc!=NULL ) {
	    for ( j=0; j<mv->glyphcnt; ++j )
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
		if ( !mv->perchar[within].selected ) {
		    MVDoSelect(mv,within);
		}
	    }
	}
	mv->pressed_y = event->u.mouse.y;
    } else if ( event->type == et_mousemove && mv->pressed ) {
	for ( i=0; i<mv->glyphcnt && !mv->perchar[i].selected; ++i );
	if ( mv->pressedwidth ) {
	    int ow = mv->perchar[i].dwidth;
	    mv->perchar[i].dwidth = rint(mv->glyphs[i].sc->vwidth*scale) + diff;
	    if ( ow!=mv->perchar[i].dwidth ) {
		for ( j=i+1; j<mv->glyphcnt; ++j )
		    mv->perchar[j].dy = mv->perchar[j-1].dy+mv->perchar[j-1].dheight+
			    mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->v,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    int ow = mv->perchar[i-1].kernafter;
	    KernPair *kp;
	    int kpoff;
	    KernClass *kc;
	    kp = mv->glyphs[i-1].kp;
	    if ( kp!=NULL )
		kpoff = kp->off;
	    else if ((kc=mv->glyphs[i-1].kc)!=NULL )
		kpoff = kc->offsets[mv->glyphs[i-1].kc_index];
	    else
		kpoff = 0;
	    kpoff = kpoff * mv->pixelsize*iscale /
			(mv->sf->descent+mv->sf->ascent);
	    mv->perchar[i-1].kernafter = kpoff + diff;
	    if ( ow!=mv->perchar[i-1].kernafter ) {
		for ( j=i; j<mv->glyphcnt; ++j )
		    mv->perchar[j].dy = mv->perchar[j-1].dy+mv->perchar[j-1].dheight+
			    mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->v,NULL,false);
	    }
	} else if ( mv->type!=mv_kernonly ) {
	    int olda = mv->activeoff;
	    BDFChar *bdfc = BDFPieceMealCheck(mv->show,mv->glyphs[i].sc->orig_pos);
	    mv->activeoff = diff;
	    MVRedrawI(mv,i,bdfc->xmin+olda,bdfc->xmax+olda);
	}
    } else if ( event->type == et_mouseup && event->u.mouse.clicks>1 &&
	    (within!=-1 || sc!=NULL)) {
	mv->pressed = false; mv->activeoff = 0;
	mv->pressedwidth = mv->pressedkern = false;
	if ( within==-1 ) within = i;
	if ( mv->bdf==NULL )
	    CharViewCreate(mv->glyphs[within].sc,mv->fv,-1);
	else
	    BitmapViewCreate(mv->bdf->glyphs[mv->glyphs[within].sc->orig_pos],mv->bdf,mv->fv,-1);
	if ( mv->showgrid==mv_hidemovinggrid )
	    GDrawRequestExpose(mv->v,NULL,false);
    } else if ( event->type == et_mouseup && mv->pressed ) {
	for ( i=0; i<mv->glyphcnt && !mv->perchar[i].selected; ++i );
	mv->pressed = false;
	mv->activeoff = 0;
	sc = mv->glyphs[i].sc;
	if ( mv->pressedwidth ) {
	    mv->pressedwidth = false;
	    diff = diff*(mv->sf->ascent+mv->sf->descent)/(mv->pixelsize*iscale);
	    if ( diff!=0 ) {
		SCPreserveWidth(sc);
		sc->vwidth += diff;
		SCCharChangedUpdate(sc,ly_none);
		for ( ; i<mv->glyphcnt; ++i )
		    mv->perchar[i].dy = mv->perchar[i-1].dy+mv->perchar[i-1].dheight +
			    mv->perchar[i-1].kernafter ;
		GDrawRequestExpose(mv->v,NULL,false);
	    } else if ( mv->showgrid==mv_hidemovinggrid )
		GDrawRequestExpose(mv->v,NULL,false);
	} else if ( mv->pressedkern ) {
	    mv->pressedkern = false;
	    diff = diff*(mv->sf->ascent+mv->sf->descent)/(mv->pixelsize*iscale);
	    if ( diff!=0 )
		MV_ChangeKerning(mv, i, diff, true);
	    MVRefreshValues(mv,i-1);
	} else if ( mv->type!=mv_kernonly ) {
	    real transform[6];
	    DBounds bb;
	    SplineCharFindBounds(sc,&bb);
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[4] = 0;
	    transform[5] = -diff*
		    (mv->sf->ascent+mv->sf->descent)/(mv->pixelsize*iscale);
	    if ( transform[5]!=0 )
		FVTrans( (FontViewBase *)mv->fv,sc,transform,NULL,false);
	}
	mv->pressedwidth = false;
	mv->pressedkern = false;
    } else if ( event->type == et_mouseup && mv->bdf!=NULL && within!=-1 ) {
	for ( j=0; j<mv->glyphcnt; ++j )
	    if ( j!=within && mv->perchar[j].selected )
		MVDeselectChar(mv,j);
	MVSelectChar(mv,within);
	if ( mv->showgrid==mv_hidemovinggrid )
	    GDrawRequestExpose(mv->v,NULL,false);
    }
}

static void MVSubMouse(MetricsView *mv,GEvent *event) {
    // This handles mouse events in the preview area.
    int i, x, y, j, within, ybase;
    SplineChar *sc;
    int diff;
    int onwidth, onkern;
    BDFChar *bdfc;
    double iscale = mv->pixelsize_set_by_window ? 1.0 : mv_scales[mv->scale_index];

    GGadgetEndPopup();

    if ( event->type==et_mouseup ) {
	event->type = et_mousemove;
	MVSubMouse(mv,event);
	event->u.mouse.x -= mv->xoff;
	event->u.mouse.y -= mv->yoff;
	event->type = et_mouseup;
    }

    event->u.mouse.x += mv->xoff;
    event->u.mouse.y += mv->yoff;
    if ( mv->vertical ) {
	_MVSubVMouse(mv,event);
return;
    }

    ybase = mv->ybaseline - mv->yoff;
    within = -1;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	x = mv->perchar[i].dx + mv->perchar[i].xoff;
	if ( mv->right_to_left )
	    x = mv->vwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
	y = ybase - mv->perchar[i].yoff;
	if ( mv->bdf==NULL ) {
	    bdfc = BDFPieceMealCheck(mv->show,mv->glyphs[i].sc->orig_pos);
	    if ( event->u.mouse.x >= x+bdfc->xmin &&
		event->u.mouse.x <= x+bdfc->xmax &&
		event->u.mouse.y <= y-bdfc->ymin &&
		event->u.mouse.y >= y-bdfc->ymax &&
		hitsbit(bdfc,rint(iscale*(event->u.mouse.x-x-bdfc->xmin)),
			rint(iscale*(bdfc->ymax-(y-event->u.mouse.y)))) )
    break;
	}
	x += mv->right_to_left ? mv->perchar[i].xoff : -mv->perchar[i].xoff;
	if ( event->u.mouse.x >= x && event->u.mouse.x < x+mv->perchar[i].dwidth+ mv->perchar[i].kernafter )
	    within = i;
    }
    if ( i==mv->glyphcnt )
	sc = NULL;
    else
	sc = mv->glyphs[i].sc;

    diff = event->u.mouse.x-mv->pressed_x;
    /*if ( mv->right_to_left ) diff = -diff;*/
    onwidth = onkern = false;
    if ( sc==NULL ) {
	if ( !mv->right_to_left ) {
	    if ( mv->type == mv_kernonly ) {
		if ( within>=0 && within+1<mv->glyphcnt &&
			event->u.mouse.x>mv->perchar[within+1].dx-3 ) {
		    onkern = true;			/* subsequent char */
		    ++within;
		} else if ( within>0 &&
			event->u.mouse.x<mv->perchar[within].dx+3 )
		    onkern = true;
	    } else if ( mv->type == mv_widthonly ) {
		if ( within>=0 && within+1<mv->glyphcnt &&
			event->u.mouse.x>mv->perchar[within+1].dx-3 )
		    onwidth = true;			/* subsequent char */
		else if ( within>=0 &&
			event->u.mouse.x>mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter-3 ) {
		    onwidth = true;
		}
	    } else {
		if ( within>0 && mv->perchar[within-1].selected &&
			event->u.mouse.x<mv->perchar[within].dx+3 )
		    onwidth = true;		/* previous char */
		else if ( within!=-1 && within+1<mv->glyphcnt &&
			mv->perchar[within+1].selected &&
			event->u.mouse.x>mv->perchar[within+1].dx-3 ) {
		    onkern = true;			/* subsequent char */
		    ++within;
		} else if ( within>0 && mv->perchar[within].selected &&
			event->u.mouse.x<mv->perchar[within].dx+3 )
		    onkern = true;
		else if ( within>=0 &&
			event->u.mouse.x>mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter-3 ) {
		    onwidth = true;
		}
	    }
	} else {
	    if ( mv->type == mv_kernonly ) {
		if ( within>=0 && within+1<mv->glyphcnt &&
			event->u.mouse.x<mv->dwidth-(mv->perchar[within+1].dx-3) ) {
		    onkern = true;			/* subsequent char */
		    ++within;
		} else if ( within>0 &&
			event->u.mouse.x>mv->dwidth-(mv->perchar[within].dx+3) )
		    onkern = true;
	    } else if ( mv->type == mv_widthonly ) {
		if ( within>=0 && within+1<mv->glyphcnt &&
			event->u.mouse.x<mv->dwidth-(mv->perchar[within+1].dx-3) )
		    onwidth = true;			/* subsequent char */
		else if ( within>=0 &&
			event->u.mouse.x<mv->dwidth-(mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter-3) ) {
		    onwidth = true;
		}
	    } else {
		if ( within>0 && mv->perchar[within-1].selected &&
			event->u.mouse.x>mv->dwidth-(mv->perchar[within].dx+3) )
		    onwidth = true;		/* previous char */
		else if ( within!=-1 && within+1<mv->glyphcnt &&
			mv->perchar[within+1].selected &&
			event->u.mouse.x<mv->dwidth-(mv->perchar[within+1].dx-3) ) {
		    onkern = true;			/* subsequent char */
		    ++within;
		} else if ( within>0 && mv->perchar[within].selected &&
			event->u.mouse.x>mv->dwidth-(mv->perchar[within].dx+3) )
		    onkern = true;
		else if ( within>=0 &&
			event->u.mouse.x<mv->dwidth-(mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter-3) ) {
		    onwidth = true;
		}
	    }
	}
    }

    if ( event->type != et_mousemove || !mv->pressed ) {
	int ct = -1;
	if ( mv->bdf!=NULL ||
		( mv->type==mv_kernonly && !onkern ) ||
		( mv->type==mv_widthonly && !onwidth )) {
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
	    sc = mv->glyphs[within].sc;
	if ( sc!=NULL )
	    SCPreparePopup(mv->gw,sc,mv->fv->b.map->remap,mv->fv->b.map->backmap[sc->orig_pos],sc->unicodeenc);
/* Don't allow any editing when displaying a bitmap font */
    } else if ( event->type == et_mousedown && mv->bdf==NULL ) {
	CVPaletteDeactivate();
	if ( sc!=NULL ) {
	    for ( j=0; j<mv->glyphcnt; ++j )
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
		if ( !mv->perchar[within].selected ) {
		    MVDoSelect(mv,within);
		}
	    }
	}
	mv->pressed_x = event->u.mouse.x;
    } else if ( event->type == et_mousemove && mv->pressed ) {
//	printf("move & pressed pressedwidth:%d pressedkern:%d type!=mv_kernonly:%d\n",mv->pressedwidth,mv->pressedkern,(mv->type!=mv_kernonly));

	for ( i=0; i<mv->glyphcnt && !mv->perchar[i].selected; ++i )
	{
	    // nothing
	}

	if ( mv->pressedwidth ) {
	    int ow = mv->perchar[i].dwidth;
	    if ( mv->right_to_left ) diff = -diff;
	    bdfc = BDFPieceMealCheck(mv->show,mv->glyphs[i].sc->orig_pos);
	    mv->perchar[i].dwidth = bdfc->width + diff;
	    if ( ow!=mv->perchar[i].dwidth ) {
		for ( j=i+1; j<mv->glyphcnt; ++j )
		    mv->perchar[j].dx = mv->perchar[j-1].dx+mv->perchar[j-1].dwidth+ mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->v,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    int ow = mv->perchar[i-1].kernafter;
	    KernPair *kp;
	    int kpoff;
	    KernClass *kc;
	    int index;
	    for ( kp = mv->glyphs[i-1].sc->kerns; kp!=NULL && kp->sc!=mv->glyphs[i].sc; kp = kp->next );
	    if ( kp!=NULL )
		kpoff = kp->off;
	    else if ((kc=SFFindKernClass(mv->sf,mv->glyphs[i-1].sc,mv->glyphs[i].sc,&index,false))!=NULL )
		kpoff = kc->offsets[index];
	    else
		kpoff = 0;
	    kpoff = kpoff * mv->pixelsize*iscale /
			(mv->sf->descent+mv->sf->ascent);
	    if ( mv->right_to_left ) diff = -diff;
	    mv->perchar[i-1].kernafter = kpoff + diff;
	    if ( ow!=mv->perchar[i-1].kernafter ) {
		for ( j=i; j<mv->glyphcnt; ++j )
		    mv->perchar[j].dx = mv->perchar[j-1].dx+mv->perchar[j-1].dwidth+ mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->v,NULL,false);
	    }
	} else if ( mv->type!=mv_kernonly ) {
	    int olda = mv->activeoff;
	    bdfc = BDFPieceMealCheck(mv->show,mv->glyphs[i].sc->orig_pos);
	    mv->activeoff = diff;
	    MVRedrawI(mv,i,bdfc->xmin+olda,bdfc->xmax+olda);
	}
    } else if ( event->type == et_mouseup && event->u.mouse.clicks>1 &&
	    (within!=-1 || sc!=NULL)) {
	mv->pressed = false; mv->activeoff = 0;
	mv->pressedwidth = mv->pressedkern = false;
	if ( within==-1 ) within = i;
	if ( mv->bdf==NULL )
	    CharViewCreate(mv->glyphs[within].sc,mv->fv,-1);
	else
	    BitmapViewCreate(mv->bdf->glyphs[mv->glyphs[within].sc->orig_pos],mv->bdf,mv->fv,-1);
	if ( mv->showgrid==mv_hidemovinggrid )
	    GDrawRequestExpose(mv->v,NULL,false);
    } else if ( event->type == et_mouseup && mv->pressed ) {
	for ( i=0; i<mv->glyphcnt && !mv->perchar[i].selected; ++i )
	{
	    // nothing
	}

	printf("mvsubmouse() mv->pressedwidth:%d \n", mv->pressedwidth );
	mv->pressed = false;
	mv->activeoff = 0;
	sc = mv->glyphs[i].sc;
	if ( mv->pressedwidth ) {
	    mv->pressedwidth = false;
	    if ( mv->right_to_left ) diff = -diff;
	    diff = diff*(mv->sf->ascent+mv->sf->descent)/(mv->pixelsize*iscale);
	    printf("mvsubmouse() diff:%d \n", diff );
	    if ( diff!=0 ) {
		SCPreserveWidth(sc);
		SCSynchronizeWidth(sc,sc->width+diff,sc->width,NULL);
		SCCharChangedUpdate(sc,ly_none);
		MV_handle_collabclient_sendRedo(mv,sc);
	    }
	} else if ( mv->pressedkern ) {
	    mv->pressedkern = false;
	    diff = diff*(mv->sf->ascent+mv->sf->descent)/(mv->pixelsize*iscale);
	    if ( diff!=0 ) {
		if ( mv->right_to_left ) diff = -diff;
		MV_ChangeKerning(mv, i, diff, true);
		MVRefreshValues(mv,i-1);
	    }
	} else if ( mv->type!=mv_kernonly ) {
	    printf("mvsubmouse() not kern only \n" );
	    MV_handle_collabclient_maybeSnapshot(mv,sc);
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[5] = 0;
	    transform[4] = diff*
		    (mv->sf->ascent+mv->sf->descent)/(mv->pixelsize*iscale);
	    if ( transform[4]!=0 )
		FVTrans( (FontViewBase *)mv->fv,sc,transform,NULL, 0 | fvt_alllayers );

	    MV_handle_collabclient_sendRedo(mv,sc);
	}
	mv->pressedwidth = false;
	mv->pressedkern = false;
	if ( mv->showgrid==mv_hidemovinggrid )
	    GDrawRequestExpose(mv->v,NULL,false);
    } else if ( event->type == et_mouseup && mv->bdf!=NULL && within!=-1 ) {
	for ( j=0; j<mv->glyphcnt; ++j )
	    if ( j!=within && mv->perchar[j].selected )
		MVDeselectChar(mv,j);
	MVSelectChar(mv,within);
	if ( mv->showgrid==mv_hidemovinggrid )
	    GDrawRequestExpose(mv->v,NULL,false);
    }
}

static void MVMouse(MetricsView *mv,GEvent *event) {
    int i;

    if ( event->u.mouse.y< mv->topend || event->u.mouse.y >= mv->displayend ) {
    // mv->displayend > mv->topend
    // This triggers when the mouse is in the data entry grid.
	if ( event->u.mouse.y >= mv->displayend &&
		event->u.mouse.y<mv->height-mv->sbh ) {
	    // This excludes the scroll bar.
	    event->u.mouse.x += (mv->coff*mv->mwidth);
	    for ( i=0; i<mv->glyphcnt; ++i ) {
		if ( event->u.mouse.x >= mv->perchar[i].mx &&
			event->u.mouse.x < mv->perchar[i].mx+mv->perchar[i].mwidth )
	    break; // This triggers only if the column has an associated character.
	    }
	    if ( i<mv->glyphcnt )
		SCPreparePopup(mv->gw,mv->glyphs[i].sc,mv->fv->b.map->remap,
			mv->fv->b.map->backmap[mv->glyphs[i].sc->orig_pos],
			mv->glyphs[i].sc->unicodeenc);
	}
	if ( mv->cursor!=ct_mypointer ) {
	    GDrawSetCursor(mv->gw,ct_mypointer);
	    mv->cursor = ct_mypointer;
	}
return;
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

    within = mv->glyphcnt;
    if ( !mv->vertical ) {
	for ( i=0; i<mv->glyphcnt; ++i ) {
	    x = mv->perchar[i].dx;
	    if ( mv->right_to_left )
		x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter ;
	    if ( ex >= x && ex < x+mv->perchar[i].dwidth+ mv->perchar[i].kernafter ) {
		within = i;
	break;
	    }
	}
    } else {
	for ( i=0; i<mv->glyphcnt; ++i ) {
	    y = mv->perchar[i].dy;
	    if ( ey >= y && ey < y+mv->perchar[i].dheight+
		    mv->perchar[i].kernafter ) {
		within = i;
	break;
	    }
	}
    }

    founds = malloc(len*sizeof(SplineChar *));	/* Will be a vast over-estimate */
    start = cnames;
    for ( i=0; *start; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	if ( (founds[i]=SFGetChar(mv->sf,-1,start))!=NULL )
	    ++i;
	*pt = ch;
	start = pt;
    }
    cnt = i;
    free( cnames );
    if ( cnt==0 ) {
        free(founds);
return;
    }
    if ( within<mv->glyphcnt )
	within = mv->glyphs[within].orig_index;
    else
	within = mv->clen;

    if ( mv->clen+cnt+1>=mv->cmax ) {
	mv->cmax = mv->clen+cnt+10;
	mv->chars = realloc(mv->chars,mv->cmax*sizeof(SplineChar *));
    }
    oldtext = _GGadgetGetTitle(mv->text);
    newtext = malloc((mv->clen+cnt+1)*sizeof(unichar_t));
    u_strcpy(newtext,oldtext);
    newtext[mv->clen+cnt]='\0';
    for ( i=mv->clen+cnt; i>=within+cnt; --i ) {
	newtext[i] = newtext[i-cnt];
	mv->chars[i] = mv->chars[i-cnt];
    }
    for ( i=within; i<within+cnt; ++i ) {
	mv->chars[i] = founds[i-within];
	newtext[i] = founds[i-within]->unicodeenc>=0 ?
		founds[i-within]->unicodeenc : MVFakeUnicodeOfSc(mv,founds[i-within]);
    }
    mv->clen += cnt;
    MVRemetric(mv);
    free(founds);

    GGadgetSetTitle(mv->text,newtext);
    free(newtext);

    GDrawRequestExpose(mv->v,NULL,false);
}

static int mv_v_e_h(GWindow gw, GEvent *event) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	GDrawSetLineWidth(gw,0);
	MVSubExpose(mv,gw,event);
      break;
      case et_char:
	MVChar(mv,event);
      break;
      case et_charup:
	  if ( event->u.chr.keysym == GK_Left || event->u.chr.keysym==GK_KP_Left
	       || event->u.chr.keysym == GK_Right || event->u.chr.keysym==GK_KP_Right ) {
	      if( event->u.chr.state&ksm_meta ) {
		  MVChar(mv,event);
	      }
	  }
      break;
      case et_mouseup: case et_mousemove: case et_mousedown:
	if (( event->type==et_mouseup || event->type==et_mousedown ) &&
		(event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	    int ish = event->u.mouse.button>5;
	    if ( event->u.mouse.state&ksm_shift ) ish = !ish;
	    if ( event->u.mouse.state&ksm_control ) {	/* bind control to magnify/minify */
		if ( event->type==et_mousedown ) {
		    if ( event->u.mouse.button==4 || event->u.mouse.button==6 )
			_MVMenuScale(mv,MID_ZoomIn);
		    else
			_MVMenuScale(mv,MID_ZoomOut);
		}
	    } else if ( ish ) {		/* bind shift to horizontal scroll */
return( GGadgetDispatchEvent(mv->hsb,event));
	    } else {
return( GGadgetDispatchEvent(mv->vsb,event));
	    }
return( true );
	}
	if ( mv->gwgic!=NULL && event->type==et_mousedown)
	    GDrawSetGIC(mv->gw,mv->gwgic,0,20);
	MVSubMouse(mv,event);
      break;
      case et_drop:
	MVDrop(mv,event);
      break;
    }
return( true );
}

static int mv_e_h(GWindow gw, GEvent *event) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf;
    GGadget *active = 0;
//    printf("mv_e_h()  event->type:%d\n", event->type );

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
	    MVResize(mv);
      break;
      case et_char:
	if ((event->u.chr.keysym == GK_Tab || event->u.chr.keysym == GK_BackTab) && (!(event->u.chr.state&ksm_meta))) {
	  // We want to allow somebody to move the cursor position
	  // forwards with tab and backwards with shift + tab.
	  // GGadget *active = GWindowGetFocusGadgetOfWindow(mv->gw); if (event->u.chr.state&ksm_shift) return 0;
	  // For now, we just return 0 so that the default event handler takes care.
	  return 0;
	}
	// MVChar(mv,event);
      break;
      case et_charup:
	if ((event->u.chr.keysym == GK_Tab || event->u.chr.keysym == GK_BackTab) && (!(event->u.chr.state&ksm_meta))) {
	  // We want to allow somebody to move the cursor position
	  // forwards with tab and backwards with shift + tab.
	  // GGadget *active = GWindowGetFocusGadgetOfWindow(mv->gw); if (event->u.chr.state&ksm_shift) return 0;
	  // For now, we just return 0 so that the default event handler takes care.
	  return 0;
	} else if ((event->u.chr.keysym == GK_Return) && (!(event->u.chr.state&ksm_meta))) {
		MVMoveInTableByColumnByOffset(mv, (event->u.chr.state&ksm_shift) ? -1 : 1);
	} else {
		MVChar(mv,event);
	}
#if 0
	  // It is unclear to Frank why we were being so selective.
	  if ( event->u.chr.keysym == GK_Left || event->u.chr.keysym==GK_KP_Left
	       || event->u.chr.keysym == GK_Right || event->u.chr.keysym==GK_KP_Right ) {
	      if( event->u.chr.state&ksm_meta ) {
		  MVChar(mv,event);
	      }
	  }
#endif // 0
      break;
      case et_mouseup: case et_mousemove: case et_mousedown:
          active = GWindowGetFocusGadgetOfWindow(mv->gw);
          if( GGadgetContainsEventLocation( mv->textPrev, event ))
          {
              GGadgetPreparePopup(mv->gw,c_to_u("Show the previous word in the current word list\n"
                                                "Select the menu File / Load Word List... to load a wordlist."));
          }
          else if( GGadgetContainsEventLocation( mv->textNext, event ))
          {
              GGadgetPreparePopup(mv->gw,c_to_u("Show the next word in the current word list\n"
                                                "Select the menu File / Load Word List... to load a wordlist."));
          }
          else if( GGadgetContainsEventLocation( mv->text, event ))
          {
              GGadgetPreparePopup(mv->gw,c_to_u("This is a word list that you can step through to quickly see your glyphs in context\n"
                                                "Select the menu File / Load Word List... to load a wordlist."));
          }
          else
          {
              GGadgetPreparePopup(mv->gw, 0);
          }
          
          
	if (( event->type==et_mouseup || event->type==et_mousedown ) &&
		(event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	    int ish = event->u.mouse.button>5;
	    if ( event->u.mouse.state&ksm_shift ) ish = !ish;
	    if ( event->u.mouse.state&ksm_control ) {	/* bind control to magnify/minify */
		if ( event->type==et_mousedown ) {
		    if ( event->u.mouse.button==4 || event->u.mouse.button==6 )
			_MVMenuScale(mv,MID_ZoomIn);
		    else
			_MVMenuScale(mv,MID_ZoomOut);
		}
	    } else if ( ish ) {	/* bind shift to horizontal scroll */
return( GGadgetDispatchEvent(mv->hsb,event));
	    } else {
return( GGadgetDispatchEvent(mv->vsb,event));
	    }
return( true );
	}
	if ( mv->gwgic!=NULL && event->type==et_mousedown)
	    GDrawSetGIC(mv->gw,mv->gwgic,0,20);
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
	sf = mv->sf;
	if ( sf->cidmaster ) sf = sf->cidmaster;
	if ( sf->metrics==mv )
	    sf->metrics = mv->next;
	else {
	    MetricsView *n;
	    for ( n=sf->metrics; n->next!=mv; n=n->next );
	    n->next = mv->next;
	}
	KCLD_MvDetach(sf->kcld,mv);
	MetricsViewFree(mv);
      break;
      case et_focus:
      break;
    }
return( true );
}

GTextInfo *SLOfFont(SplineFont *sf) {
    uint32 *scripttags, *langtags;
    int s, l, i, k, cnt;
    extern GTextInfo scripts[], languages[];
    GTextInfo *ret = NULL;
    char *sname, *lname, *temp;
    char sbuf[8], lbuf[8];

    LookupUIInit();
    scripttags = SFScriptsInLookups(sf,-1);
    if ( scripttags==NULL )
return( NULL );

    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( s=0; scripttags[s]!=0; ++s ) {
	    if ( k ) {
		for ( i=0; scripts[i].text!=NULL; ++i )
		    if ( scripttags[s] == (intpt) (scripts[i].userdata))
		break;
		sname = (char *) (scripts[i].text);
		sbuf[0] = scripttags[s]>>24;
		sbuf[1] = scripttags[s]>>16;
		sbuf[2] = scripttags[s]>>8;
		sbuf[3] = scripttags[s];
		sbuf[4] = 0;
		if ( sname==NULL )
		    sname = sbuf;
	    }
	    langtags = SFLangsInScript(sf,-1,scripttags[s]);
	    /* This one can't be NULL */
	    for ( l=0; langtags[l]!=0; ++l ) {
		if ( k ) {
		    for ( i=0; languages[i].text!=NULL; ++i )
			if ( langtags[l] == (intpt) (languages[i].userdata))
		    break;
		    lname = (char *) (languages[i].text);
		    lbuf[0] = langtags[l]>>24;
		    lbuf[1] = langtags[l]>>16;
		    lbuf[2] = langtags[l]>>8;
		    lbuf[3] = langtags[l];
		    lbuf[4] = 0;
		    if ( lname==NULL )
			lname = lbuf;
		    temp = malloc(strlen(sname)+strlen(lname)+3);
		    strcpy(temp,sname); strcat(temp,"{"); strcat(temp,lname); strcat(temp,"}");
		    ret[cnt].text = (unichar_t *) temp;
		    ret[cnt].text_is_1byte = true;
		    temp = malloc(11);
		    strcpy(temp,sbuf); temp[4] = '{'; strcpy(temp+5,lbuf); temp[9]='}'; temp[10] = 0;
		    ret[cnt].userdata = temp;
		}
		++cnt;
	    }
	    free(langtags);
	}
	if ( !k )
	    ret = calloc((cnt+1),sizeof(GTextInfo));
    }
    free(scripttags);
return( ret );
}

#define metricsicon_width 16
#define metricsicon_height 16
static unsigned char metricsicon_bits[] = {
   0x04, 0x10, 0xf0, 0x03, 0x24, 0x12, 0x20, 0x00, 0x24, 0x10, 0xe0, 0x00,
   0x24, 0x10, 0x20, 0x00, 0x24, 0x10, 0x20, 0x00, 0x74, 0x10, 0x00, 0x00,
   0x55, 0x55, 0x00, 0x00, 0x04, 0x10, 0x00, 0x00};

static int metricsview_ready = false;

static void MetricsViewFinish() {
  if (!metricsview_ready) return;
  mb2FreeGetText(mblist);
}

void MetricsViewFinishNonStatic() {
  MetricsViewFinish();
}

static void MetricsViewInit(void ) {
    // static int inited = false; // superseded by metricsview_ready.
    if (metricsview_ready) return;
	mv_text_init[2].text = (unichar_t *) _((char *) mv_text_init[2].text);
	mb2DoGetText(mblist);
	MVColInit();
    atexit(&MetricsViewFinishNonStatic);
}

MetricsView *MetricsViewCreate(FontView *fv,SplineChar *sc,BDFFont *bdf) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GRect gsize;
    MetricsView *mv = calloc(1,sizeof(MetricsView));
    FontRequest rq;
    static GWindow icon = NULL;
    extern int _GScrollBar_Width;
    // Max. glyphname length: 31, max. chars picked up: 15. 31*15 = 465
    char buf[465], *pt;
    GTextInfo label;
    int i,j,cnt;
    int as,ds,ld;
    static GFont *mvfont=NULL;
    SplineFont *master = fv->b.sf->cidmaster ? fv->b.sf->cidmaster : fv->b.sf;

    MetricsViewInit();

    if ( icon==NULL )
	icon = GDrawCreateBitmap(NULL,metricsicon_width,metricsicon_height,metricsicon_bits);

    mv->fv = fv;
    mv->sf = fv->b.sf;
    mv->bdf = bdf;
    mv->showgrid = mvshowgrid;
    mv->antialias = mv_antialias;
    mv->scale_index = SCALE_INDEX_NORMAL;
    mv->next = master->metrics;
    master->metrics = mv;
    mv->layer = fv->b.active_layer;
    mv->type = mv_type;
    mv->pixelsize_set_by_window = true;
    mv->dpi = 72;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_icon;
    wattrs.event_masks = ~(0);
    wattrs.cursor = ct_mypointer;
    MVWindowTitle(buf,sizeof(buf),mv);
    wattrs.utf8_window_title = buf;
    wattrs.icon = icon;
    pos.x = pos.y = 0;
    pos.width = mv_width;
    pos.height = mv_height;
    mv->gw = gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,mv,&wattrs);
    mv->width = pos.width; mv->height = pos.height;
    mv->gwgic = GDrawCreateInputContext(mv->gw,gic_root|gic_orlesser);
    GDrawSetGIC(gw,mv->gwgic,0,20);
    GDrawSetWindowTypeName(mv->gw, "MetricsView");

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = MVMenuContextualHelp;
    gd.u.menu2 = mblist;
    mv->mb = GMenu2BarCreate( gw, &gd, NULL);
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

    if ( mvfont==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = -12;
	rq.weight = 400;
	mvfont = GDrawInstanciateFont(gw,&rq);
	mvfont = GResourceFindFont("MetricsView.Font",mvfont);
    }
    mv->font = mvfont;
    GDrawWindowFontMetrics(gw,mv->font,&as,&ds,&ld);
    mv->fh = as+ds; mv->as = as;

    pt = buf;
    mv->chars = calloc(mv->cmax=20,sizeof(SplineChar *));
    if ( sc!=NULL ) {
	mv->chars[mv->clen++] = sc;
    } else {
	EncMap *map = fv->b.map;
	for ( j=1; (j<=fv->sel_index || j<1) && mv->clen<15; ++j ) {
	    for ( i=0; i<map->enccount && mv->clen<15; ++i ) {
		int gid = map->map[i];
		if ( gid!=-1 && fv->b.selected[i]==j && fv->b.sf->glyphs[gid]!=NULL ) {
		    mv->chars[mv->clen++] = fv->b.sf->glyphs[gid];
		}
	    }
	}
    }
    mv->chars[mv->clen] = NULL;

    for ( cnt=0; cnt<mv->clen; ++cnt ) {
        if ( mv->chars[cnt]->unicodeenc != -1 )
	    pt = utf8_idpb(pt,mv->chars[cnt]->unicodeenc,0);
        else {
            *pt = '/'; pt++;
            strcpy(pt, mv->chars[cnt]->name);
            pt += strlen(mv->chars[cnt]->name);
            *pt = ' '; pt++;
        }
    }
    *pt = '\0';

    memset(&gd,0,sizeof(gd));
    memset(&label,0,sizeof(label));
    gd.pos.y = mv->mbh+2; gd.pos.x = 10;
    gd.pos.width = GDrawPointsToPixels(mv->gw,100);
    gd.label = &label;
    label.text = (unichar_t *) "DFLT{dflt}";
    label.text_is_1byte = true;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gd.u.list = mv->scriptlangs = SLOfFont(mv->sf);
    gd.handle_controlevent = MV_ScriptLangChanged;
    mv->script = GListFieldCreate(gw,&gd,mv);
    GGadgetGetSize(mv->script,&gsize);
    mv->topend = gsize.y + gsize.height + 2;

    gd.pos.x = gd.pos.x+gd.pos.width+10;
    gd.pos.width = GDrawPointsToPixels(mv->gw,200);
    gd.pos.height = gsize.height;
    label.text = (unichar_t *) buf;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_text_xim;
    gd.handle_controlevent = MV_TextChanged;
    gd.u.list = mv_text_init;
    mv->text = GListFieldCreate(gw,&gd,mv);

    // Up and Down buttons for moving through the word list.
    {
        GTextInfo label[9];
        GGadgetData xgd = gd;
        gd.pos.width += 2 * xgd.pos.height + 4;
        memset(label, '\0', sizeof(GTextInfo));
        xgd.pos.x += xgd.pos.width + 2;
        xgd.pos.width = xgd.pos.height;
        xgd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
        xgd.handle_controlevent = MVMoveToPrevInWordList;
        xgd.label = &label[0];
        label[0].text = (unichar_t *) "⇞";
        label[0].text_is_1byte = true;
        mv->textPrev = GButtonCreate(mv->gw,&xgd,mv);
        memset(label, '\0', sizeof(GTextInfo));
        xgd.pos.x += xgd.pos.width + 2;
        xgd.pos.width = xgd.pos.height;
        xgd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
        xgd.handle_controlevent = MVMoveToNextInWordList;
        xgd.label = &label[0];
        label[0].text = (unichar_t *) "⇟";
        label[0].text_is_1byte = true;
        mv->textNext = GButtonCreate(mv->gw,&xgd,mv);
    }
    

    gd.pos.x = gd.pos.x+gd.pos.width+10; --gd.pos.y;
    gd.pos.width += 30;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gd.handle_controlevent = MV_SubtableChanged;
    gd.label = NULL;
    gd.u.list = NULL;
    mv->subtable_list = GListButtonCreate(gw,&gd,mv);
    MVSetSubtables(master);

    gd.pos.y = mv->topend; gd.pos.x = 0;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_pos_use0|gg_list_multiplesel|gg_list_alphabetic;
    gd.pos.width = GDrawPointsToPixels(mv->gw,50);
    gd.handle_controlevent = MV_FeaturesChanged;
    mv->features = GListCreate(gw,&gd,mv);
    GListSetSBAlwaysVisible(mv->features,true);
    GListSetPopupCallback(mv->features,MV_FriendlyFeatures);
    mv->xstart = gd.pos.width;

    pos.x = mv->xstart; pos.width = mv->dwidth - mv->xstart;
    pos.y = mv->topend+2; pos.height = mv->displayend - mv->topend - 2;
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_backcol;
    wattrs.background_color = view_bgcol;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    mv->v = GWidgetCreateSubWindow(mv->gw,&pos,mv_v_e_h,mv,&wattrs);
    GDrawSetWindowTypeName(mv->v, "MetricsView");

    MVSetFeatures(mv);
    MVMakeLabels(mv);
    MVResize(mv);

    GDrawSetVisible(mv->v,true);
    GDrawSetVisible(gw,true);
    /*GWidgetHidePalettes();*/
return( mv );
}

void MetricsViewFree(MetricsView *mv) {

    if ( mv->scriptlangs!=NULL ) {
	int i;
	for ( i=0; mv->scriptlangs[i].text!=NULL ; ++i )
	    free(mv->scriptlangs[i].userdata );
	GTextInfoListFree(mv->scriptlangs);
    }
    BDFFontFree(mv->show);
    /* the fields will free themselves */
    free(mv->chars);
    free(mv->glyphs);
    free(mv->perchar);
    free(mv);
}

void MVRefreshAll(MetricsView *mv) {

    if ( mv!=NULL ) {
	MVRemetric(mv);
	GDrawRequestExpose(mv->v,NULL,false);
    }
}

/******************************************************************************/
static int MV_GlyphCnt(struct metricsview *mv) {
return( mv->glyphcnt );
}

static SplineChar *MV_Glyph(struct metricsview *mv,int i) {
    if ( i<0 || i>=mv->glyphcnt )
return( NULL );

return( mv->glyphs[i].sc );
}

static void MV_ReKernAll(struct splinefont *sf) {
    MetricsView *mv;

    for ( mv=sf->metrics; mv!=NULL; mv=mv->next )
	MVReKern(mv);
}

static void MV_ReFeatureAll(struct splinefont *sf) {
    MetricsView *mv;

    MVSetSubtables(sf);
    for ( mv=sf->metrics; mv!=NULL; mv=mv->next )
	MVSetFeatures(mv);
}

static void MV_CloseAll(struct splinefont *sf) {
    MetricsView *mv, *mvnext;
    for ( mv=sf->metrics; mv!=NULL; mv=mvnext ) {
	mvnext = mv->next;
	GDrawDestroyWindow(mv->gw);
    }
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

struct mv_interface gdraw_mv_interface = {
    MV_GlyphCnt,
    MV_Glyph,
    MV_ReKernAll,
    MV_ReFeatureAll,
    MV_CloseAll
};

static struct resed metricsview_re[] = {
    {N_("Advance Width Col"), "AdvanceWidthColor", rt_color, &widthcol, N_("Color used to draw the advance width line of a glyph"), NULL, { 0 }, 0, 0 },
    {N_("Italic Advance Col"), "ItalicAdvanceColor", rt_color, &widthcol, N_("Color used to draw the italic advance width line of a glyph"), NULL, { 0 }, 0, 0 },
    {N_("Kern Line Color"), "KernLineColor", rt_color, &kernlinecol, N_("Color used to draw the kerning line"), NULL, { 0 }, 0, 0 },
    {N_("Side Bearing Color"), "SideBearingLineColor", rt_color, &rbearinglinecol, N_("Color used to draw the left side bearing"), NULL, { 0 }, 0, 0 },
    {N_("Selected Glyph Col"), "SelectedGlyphColor", rt_color, &selglyphcol, N_("Color used to mark the selected glyph"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
extern GResInfo view_ri;
GResInfo metricsview_ri = {
    &view_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    metricsview_re,
    N_("MetricsView"),
    N_("This window displays metrics information about a font"),
    "MetricsView",
    "fontforge",
    false,
    0,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};


void MVSelectFirstKerningTable(struct metricsview *mv)
{
    /* SplineFont *sf = mv->sf; */
    /* printf("MVSelectFirstKerningTable() kerns:%p\n", sf->kerns ); */
    /* if( sf->kerns ) */
    /* { */
    /* 	printf("MVSelectFirstKerningTable() kerns.next:%p\n", sf->kerns->next ); */
    /* 	printf("MVSelectFirstKerningTable() kerns.subt:%p\n", sf->kerns->subtable ); */
    /* } */

    //
    // if nothing selected, then select the first entry.
    //
    if( GGadgetGetFirstListSelectedItem(mv->features) >= 0 )
    {
	return;
    }

    GTextInfo **ti=NULL;
    int32 len;
    ti = GGadgetGetList(mv->features,&len);
    GGadgetSelectOneListItem(mv->features,0);
    MVRemetric(mv);
    GDrawRequestExpose(mv->v,NULL,false);
}


