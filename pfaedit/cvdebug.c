/* Copyright (C) 2003 by George Williams */
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
#include "pfaeditui.h"

#ifndef FREETYPE_HAS_DEBUGGER
void CVDebugReInit(CharView *cv,int restart_debug) {
}

void CVDebugFree(DebugView *dv) {
}
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#include "ttinterp.h"

static int DVBpCheck(struct instrinfo *ii, int ip) {
    DebugView *dv = (ii->userdata);
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);

    if ( exc==NULL )
return( false );

return( DebuggerBpCheck( dv->dc,exc->curRange,ip));
}

static void DVToggleBp(struct instrinfo *ii, int ip) {
    DebugView *dv = (ii->userdata);
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);

    if ( exc==NULL )
return;
    DebuggerToggleBp(dv->dc,exc->curRange,ip);
}

static SplineSet *SplineSetsFromPoints(TT_GlyphZoneRec *pts, real scale) {
    int i=0, c, last_off, start;
    SplineSet *head=NULL, *last=NULL, *cur;
    SplinePoint *sp;
    /* very similar to parsettf.c: ttfbuildcontours */

    for ( c=0; c<pts->n_contours; ++c ) {
	if ( pts->contours[c]<i )	/* Sigh. Yes there are fonts with bad endpt info */
    continue;
	cur = chunkalloc(sizeof(SplineSet));
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	last_off = false;
	start = i;
	while ( i<=pts->contours[c] ) {
	    if ( pts->tags[i]&FT_Curve_Tag_On ) {
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me.x = sp->nextcp.x = sp->prevcp.x = pts->cur[i].x*scale;
		sp->me.y = sp->nextcp.y = sp->prevcp.y = pts->cur[i].y*scale;
		sp->nonextcp = sp->noprevcp = true;
		sp->ttfindex = i;
		if ( last_off && cur->last!=NULL ) {
		    cur->last->nextcp.x = sp->prevcp.x = pts->cur[i-1].x*scale;
		    cur->last->nextcp.y = sp->prevcp.y = pts->cur[i-1].y*scale;
		}
		last_off = false;
	    } else if ( last_off ) {
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me.x = (pts->cur[i].x+pts->cur[i-1].x)/2 * scale;
		sp->me.y = (pts->cur[i].y+pts->cur[i-1].y)/2 * scale;
		sp->nextcp = sp->prevcp = sp->me;
		sp->nonextcp = true;
		sp->ttfindex = 0xffff;
		if ( last_off && cur->last!=NULL ) {
		    cur->last->nextcp.x = sp->prevcp.x = pts->cur[i-1].x * scale;
		    cur->last->nextcp.y = sp->prevcp.y = pts->cur[i-1].y * scale;
		}
		/* last_off continues to be true */
	    } else {
		last_off = true;
		sp = NULL;
	    }
	    if ( sp!=NULL ) {
		if ( cur->first==NULL )
		    cur->first = sp;
		else
		    SplineMake2(cur->last,sp);
		cur->last = sp;
	    }
	    ++i;
	}
	if ( start==i-1 ) {
	    /* MS chinese fonts have contours consisting of a single off curve*/
	    /*  point. What on earth do they think that means? */
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = pts->cur[start].x * scale;
	    sp->me.y = pts->cur[start].y * scale;
	    sp->nextcp = sp->prevcp = sp->me;
	    sp->nonextcp = sp->noprevcp = true;
	    sp->ttfindex = i-1;
	    cur->first = cur->last = sp;
	} else if ( !(pts->tags[start]&FT_Curve_Tag_On) && !(pts->tags[i-1]&FT_Curve_Tag_On) ) {
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = (pts->cur[start].x+pts->cur[i-1].x)/2 * scale;
	    sp->me.y = (pts->cur[start].y+pts->cur[i-1].y)/2 * scale;
	    sp->nextcp = sp->prevcp = sp->me;
	    sp->nonextcp = true;
	    sp->ttfindex = 0xffff;
	    cur->last->nextcp.x = sp->prevcp.x = pts->cur[i-1].x * scale;
	    cur->last->nextcp.y = sp->prevcp.y = pts->cur[i-1].y * scale;
	    SplineMake2(cur->last,sp);
	    cur->last = sp;
	    cur->last->nextcp.x = cur->first->prevcp.x = pts->cur[start].x * scale;
	    cur->last->nextcp.y = cur->first->prevcp.y = pts->cur[start].y * scale;
	} else if ( !(pts->tags[i-1]&FT_Curve_Tag_On)) {
	    cur->last->nextcp.x = cur->first->prevcp.x = pts->cur[i-1].x * scale;
	    cur->last->nextcp.y = cur->first->prevcp.y = pts->cur[i-1].y * scale;
	} else if ( !(pts->tags[start]&FT_Curve_Tag_On) ) {
	    cur->last->nextcp.x = cur->first->prevcp.x = pts->cur[start].x * scale;
	    cur->last->nextcp.y = cur->first->prevcp.y = pts->cur[start].y * scale;
	}
	if ( cur->last!=cur->first ) {
	    SplineMake2(cur->last,cur->first);
	    cur->last = cur->first;
	}
    }
return( head );
}

static void DVFigureNewState(DebugView *dv,TT_ExecContext exc) {

    /* Code to look for proper function/idef rather than the full fpgm table */
    if ( exc==NULL ) {
	dv->id.instrs = NULL;
	dv->id.instr_cnt = 0;
	IIReinit(&dv->ii,-1);
    } else if ( dv->id.instrs!=(uint8 *) exc->code ) {
	dv->id.instrs =(uint8 *) exc->code;
	dv->id.instr_cnt = exc->codeSize;
	IIReinit(&dv->ii,exc->IP);
    } else
	IIScrollTo(&dv->ii,exc->IP,true);

    if ( exc!=NULL ) {
	SplinePointListsFree(dv->cv->gridfit);
	dv->cv->gridfit = SplineSetsFromPoints(&exc->pts,dv->scale);
	dv->cv->ft_gridfitwidth = exc->pts.cur[exc->pts.n_points-1].x * dv->scale;
	GDrawRequestExpose(dv->cv->v,NULL,false);
    }
}

static int DV_Run(GGadget *g, GEvent *e) {
    DebugView *dv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	dv = GDrawGetUserData(GGadgetGetWindow(g));
	DebuggerGo(dv->dc,(enum debug_gotype) GGadgetGetCid(g));
	DVFigureNewState(dv,DebuggerGetEContext(dv->dc));
    }
return( true );
}

static void DVExpose(GWindow pixmap, DebugView *dv,GEvent *event) {
    GDrawDrawLine(pixmap, 0, dv->toph-1, dv->dwidth, dv->toph-1, 0x000000 );
#if 0
    GDrawDrawImage(pixmap, &GIcon_stepinto, NULL, 2, 2);
    GDrawDrawImage(pixmap, &GIcon_stepover, NULL, 1*(2+16)+2, 2);
    GDrawDrawImage(pixmap, &GIcon_stepout, NULL, 2*(2+16)+2, 2);
    GDrawDrawImage(pixmap, &GIcon_continue, NULL, 3*(2+16)+2, 2);
#endif
}

static int dv_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	DVExpose(gw,dv,event);
      break;
      case et_char:
      break;
      case et_charup:
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    instr_scroll(&dv->ii,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    /*DVResize(dv,event)*/;
      break;
      case et_destroy:
	dv->dv = NULL;
	free(dv->id.bts);
      break;
      case et_mouseup: case et_mousedown:
	GGadgetEndPopup();
      break;
      case et_mousemove:
      break;
    }
return( true );
}

void CVDebugFree(DebugView *dv) {
    if ( dv!=NULL ) {
	CharView *cv = dv->cv;
	cv->show_ft_results = false;
	DebuggerTerminate(dv->dc);
	cv->dv = NULL;
	if ( dv->dv!=NULL ) {
	    GDrawDestroyWindow(dv->dv);
	    CVResize(cv);
	    GDrawRequestExpose(cv->v,NULL,false);
	}
	free(dv);
    }
}

void CVDebugReInit(CharView *cv,int restart_debug,int dbg_fpgm) {
    DebugView *dv = cv->dv;
    GWindowAttrs wattrs;
    GRect pos;
    TT_ExecContext exc;
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    int as,ds,ld;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    extern int _GScrollBar_Width;
    double scale;

    if ( restart_debug )
	scale = (cv->sc->parent->ascent+cv->sc->parent->descent)/(cv->ft_pointsize*cv->ft_dpi/72.0) / (1<<6);
    if ( !restart_debug ) {
	CVDebugFree(dv);
    } else if ( dv==NULL ) {
	int sbsize = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
	cv->show_ft_results = false;
	cv->dv = dv = gcalloc(1,sizeof(DebugView));
	dv->dwidth = 260;
	dv->scale = scale;
	dv->cv = cv;
	dv->dc = DebuggerCreate(cv->sc,cv->ft_pointsize,cv->ft_dpi,dbg_fpgm);
	if ( dv->dc==NULL ) {
	    free(dv);
	    cv->dv = NULL;
return;
	}
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_bordwidth|wam_bordcol;
	wattrs.event_masks = -1;
	wattrs.border_width = 0;
	wattrs.border_color = 0x000000;
	wattrs.cursor = ct_mypointer;
	pos.x = 0; pos.y = 0;
	pos.width = dv->dwidth; pos.height = cv->height;
	dv->dv = GWidgetCreateSubWindow(cv->gw,&pos,dv_e_h,dv,&wattrs);
	GDrawSetVisible(dv->dv,true);

	dv->toph = 36;
	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));
	gcd[0].gd.pos.y = dv->toph; gcd[0].gd.pos.height = pos.height-dv->toph;
	gcd[0].gd.pos.width = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
	gcd[0].gd.pos.x = pos.width-gcd[0].gd.pos.width;
	gcd[0].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
	gcd[0].creator = GScrollBarCreate;

	gcd[1].gd.pos.y = 2; gcd[1].gd.pos.x = 2;
	gcd[1].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	gcd[1].gd.cid = dgt_step;
	gcd[1].gd.label = &label[1];
	label[1].image = &GIcon_stepinto;
	gcd[1].gd.handle_controlevent = DV_Run;
	gcd[1].gd.popup_msg = GStringGetResource(_STR_StepPopup,NULL);
	gcd[1].creator = GButtonCreate;

	gcd[2].gd.pos.y = 2; gcd[2].gd.pos.x = 38;
	gcd[2].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	gcd[2].gd.cid = dgt_next;
	gcd[2].gd.label = &label[2];
	label[2].image = &GIcon_stepover;
	gcd[2].gd.handle_controlevent = DV_Run;
	gcd[2].gd.popup_msg = GStringGetResource(_STR_NextPopup,NULL);
	gcd[2].creator = GButtonCreate;

	gcd[3].gd.pos.y = 2; gcd[3].gd.pos.x = 74;
	gcd[3].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	gcd[3].gd.cid = dgt_stepout;
	gcd[3].gd.label = &label[3];
	label[3].image = &GIcon_stepout;
	gcd[3].gd.handle_controlevent = DV_Run;
	gcd[3].gd.popup_msg = GStringGetResource(_STR_StepOutOfPopup,NULL);
	gcd[3].creator = GButtonCreate;

	gcd[4].gd.pos.y = 2; gcd[4].gd.pos.x = 110;
	gcd[4].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	gcd[4].gd.cid = dgt_continue;
	gcd[4].gd.label = &label[4];
	label[4].image = &GIcon_continue;
	gcd[4].gd.handle_controlevent = DV_Run;
	gcd[4].gd.popup_msg = GStringGetResource(_STR_ContinuePopup,NULL);
	gcd[4].creator = GButtonCreate;

	GGadgetsCreate(dv->dv,gcd);

	dv->ii.vsb = gcd[0].ret;
	dv->ii.sbw = gcd[0].gd.pos.width;
	dv->ii.vheight = gcd[0].gd.pos.height; dv->ii.vwidth = pos.width-sbsize;
	dv->ii.showaddr = true;
	dv->ii.userdata = dv;
	dv->ii.selection_callback = DVToggleBp;
	dv->ii.bpcheck = DVBpCheck;

	pos.y = dv->toph;
	pos.width -= sbsize; pos.height -= pos.y;
	dv->ii.v = GWidgetCreateSubWindow(dv->dv,&pos,ii_v_e_h,&dv->ii,&wattrs);
	dv->ii.instrdata = &dv->id;

	memset(&rq,0,sizeof(rq));
	rq.family_name = monospace;
	rq.point_size = -12;
	rq.weight = 400;
	dv->ii.gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(cv->gw),&rq);
	GDrawSetFont(dv->ii.v,dv->ii.gfont);
	GDrawFontMetrics(dv->ii.gfont,&as,&ds,&ld);
	dv->ii.as = as+1;
	dv->ii.fh = dv->ii.as+ds;
	dv->ii.isel_pos = -1;

	if (( exc = DebuggerGetEContext(dv->dc))!=NULL )
	    DVFigureNewState(dv,exc);
	GDrawSetVisible(dv->dv,true);
	GDrawSetVisible(dv->ii.v,true);
	CVResize(cv);
	GDrawRequestExpose(cv->v,NULL,false);
    } else {
	dv->scale = scale;
	DebuggerReset(dv->dc,cv->ft_pointsize,cv->ft_dpi,dbg_fpgm);
	if (( exc = DebuggerGetEContext(dv->dc))!=NULL )
	    DVFigureNewState(dv,exc);
    }
}
#endif

