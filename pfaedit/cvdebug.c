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
#include <gkeysym.h>
#include <ustring.h>

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

static void DVRegExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    char buffer[100];
    unichar_t ubuffer[100];
    int y;

    GDrawFillRect(pixmap,&event->u.expose.rect,0xb0b0b0);
    if ( exc==NULL )
return;
    GDrawSetFont(pixmap,dv->ii.gfont);
    y = 3+dv->ii.as;

    sprintf( buffer, " rp0: %d", exc->GS.rp0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, " rp1: %d", exc->GS.rp1 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, " rp2: %d", exc->GS.rp2 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "loop: %d", exc->GS.loop ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    y+=2;

    sprintf( buffer, " zp0: %s", exc->GS.gep0?"Normal":"Twilight" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, " zp1: %s", exc->GS.gep1?"Normal":"Twilight" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, " zp2: %s", exc->GS.gep2?"Normal":"Twilight" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    y+=2;

    sprintf( buffer, "MinDist: %.2f", exc->GS.minimum_distance/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "CvtCutin: %.2f", exc->GS.control_value_cutin/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SingWidCut: %.2f", exc->GS.single_width_cutin/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SingWidVal: %.2f", exc->GS.single_width_value/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    y+=2;

    sprintf( buffer, "freeVec: %g,%g", (((int)exc->GS.freeVector.x<<16)>>(16+14)) + ((exc->GS.freeVector.x&0x3fff)/16384.0),
	    (((int)exc->GS.freeVector.y<<16)>>(16+14)) + ((exc->GS.freeVector.y&0x3fff)/16384.0) ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "projVec: %g,%g", (((int)exc->GS.projVector.x<<16)>>(16+14)) + ((exc->GS.projVector.x&0x3fff)/16384.0),
	    (((int)exc->GS.projVector.y<<16)>>(16+14)) + ((exc->GS.projVector.y&0x3fff)/16384.0) ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "dualVec: %g,%g", (((int)exc->GS.dualVector.x<<16)>>(16+14)) + ((exc->GS.dualVector.x&0x3fff)/16384.0),
	    (((int)exc->GS.dualVector.y<<16)>>(16+14)) + ((exc->GS.dualVector.y&0x3fff)/16384.0) ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    y+=2;

    sprintf( buffer, "AutoFlip: %s", exc->GS.auto_flip?"True": "False" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "DeltaBase: %d", exc->GS.delta_base ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "DeltaShift: %d", exc->GS.delta_shift ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "RndState: %s",
	    exc->GS.round_state==TT_Round_To_Half_Grid? "To Half Grid" :
	    exc->GS.round_state==TT_Round_To_Grid? "To Grid" :
	    exc->GS.round_state==TT_Round_To_Double_Grid? "To Double Grid" :
	    exc->GS.round_state==TT_Round_Down_To_Grid? "Down To Grid" :
	    exc->GS.round_state==TT_Round_Up_To_Grid? "Up To Grid" :
	    exc->GS.round_state==TT_Round_Off? "Off" :
	    exc->GS.round_state==TT_Round_Super? "Super" :
	    exc->GS.round_state==TT_Round_Super_45? "Super45" :
		"Unknown" );
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SRndPeriod: %.2f", exc->period/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SRndPhase: %.2f", exc->phase/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SRndThreshold: %.2f", exc->threshold/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "InstrControl: %d", exc->GS.instruct_control ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "ScanControl: %s", exc->GS.scan_control?"True": "False" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "ScanType: %d", exc->GS.scan_type ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;

    /* Instruction control, scan control, scan type, phase, threshold for super rounding */
}

static void DVStackExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    char buffer[100];
    unichar_t ubuffer[100];
    int i, y;

    GDrawFillRect(pixmap,&event->u.expose.rect,0xb0b0b0);
    if ( exc==NULL )
return;
    GDrawSetFont(pixmap,dv->ii.gfont);
    y = 3+dv->ii.as;
    for ( i=exc->top-1; i>=0; --i ) {
	sprintf(buffer, "%3d: %3d (0x%x)", i, exc->stack[i], exc->stack[i] );
	uc_strcpy(ubuffer,buffer);
	GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
	y += dv->ii.fh;
    }
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
	if ( dv->regs!=NULL )
	    GDrawRequestExpose(dv->regs,NULL,false);
	if ( dv->stack!=NULL )
	    GDrawRequestExpose(dv->stack,NULL,false);
    }
}

static void DVGoFigure(DebugView *dv,enum debug_gotype go) {
    DebuggerGo(dv->dc,go);
    DVFigureNewState(dv,DebuggerGetEContext(dv->dc));
}

static int DV_Run(GGadget *g, GEvent *e) {
    DebugView *dv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	dv = GDrawGetUserData(GGadgetGetWindow(g));
	DVGoFigure(dv,(enum debug_gotype) GGadgetGetCid(g));
    }
return( true );
}

static int DV_WatchPnt(GGadget *g, GEvent *e) {
    DebugView *dv;
    int pnum=0, n, any=0;
    SplineSet *ss;
    SplinePoint *sp;
    uint8 *watches;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	dv = GDrawGetUserData(GGadgetGetWindow(g));
	if ( dv->cv->sc->refs!=NULL ) {
	    GWidgetErrorR(_STR_NoWatchPoints,_STR_NoWatchPointsWithRefs);
return( true );
	}

	DebuggerGetWatches(dv->dc,&n);
	watches = gcalloc(n,sizeof(uint8));

	for ( ss = dv->cv->sc->splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first; ; ) {
		if ( sp->ttfindex == 0xffff )
		    /* Ignore it */;
		else if ( sp->selected && sp->ttfindex<n) {
		    watches[pnum=sp->ttfindex] = true;
		    any = true;
		}
		if ( !sp->nonextcp ) {
		    ++pnum;
		    if ( sp==dv->cv->p.sp && dv->cv->p.nextcp && pnum<n )
			watches[pnum] = true;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
	if ( !any ) {
	    free(watches);
	    watches = NULL;
	}
	DebuggerSetWatches(dv->dc,n,watches);
    }
return( true );
}

static void DVExpose(GWindow pixmap, DebugView *dv,GEvent *event) {
    GDrawDrawLine(pixmap, 0, dv->toph-1, dv->dwidth, dv->toph-1, 0x000000 );
}

int DVChar(DebugView *dv, GEvent *event) {

    if ( event->u.chr.state&(ksm_control|ksm_meta) || dv==NULL )
return( false );
    /* Can't redo F1, handled by menu */
    if ( event->u.chr.keysym == GK_Help )
return( false );
    if ( event->u.chr.keysym >= GK_F1 && event->u.chr.keysym<=GK_F10 )
return( false );

    switch ( event->u.chr.keysym ) {
      case 's': case 'S':
	DVGoFigure(dv,dgt_step);
      break;
      case 'n': case 'N':
	DVGoFigure(dv,dgt_next);
      break;
      case 'r': case 'R':
	DVGoFigure(dv,dgt_stepout);
      break;
      case 'c': case 'C':
	DVGoFigure(dv,dgt_continue);
      break;
    }
return( true );
}

static int dvreg_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	DVRegExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_close:
	GDrawDestroyWindow(dv->regs);
      break;
      case et_destroy:
	dv->regs = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static int dvstack_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	DVStackExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
#if 0
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    stack_scroll(dv,&event->u.control.u.sb);
	  break;
	}
      break;
#endif
      case et_close:
	GDrawDestroyWindow(dv->stack);
      break;
      case et_destroy:
	dv->stack = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static void DVCreateRegs(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.window_title = GStringGetResource(_STR_TTRegisters,NULL);
    pos.x = 0; pos.y = 0;
    pos.width = 133; pos.height = 269;
    dv->regs = GDrawCreateTopWindow(NULL,&pos,dvreg_e_h,dv,&wattrs);
    GDrawSetVisible(dv->regs,true);
}

static void DVCreateStack(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;
    /*extern int _GScrollBar_Width;*/

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.window_title = GStringGetResource(_STR_TTStack,NULL);
    pos.x = 0; pos.y = 0;
    pos.width = 133; pos.height = 269;
    dv->stack = GDrawCreateTopWindow(NULL,&pos,dvstack_e_h,dv,&wattrs);
    GDrawSetVisible(dv->stack,true);
}

static int dv_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	DVExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
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

	gcd[5].gd.pos.y = 2; gcd[5].gd.pos.x = 146;
	gcd[5].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	/*gcd[5].gd.cid = dgt_continue;*/
	gcd[5].gd.label = &label[5];
	label[5].image = &GIcon_watchpnt;
	gcd[5].gd.handle_controlevent = DV_WatchPnt;
	gcd[5].gd.popup_msg = GStringGetResource(_STR_WatchPointPopup,NULL);
	gcd[5].creator = GButtonCreate;

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
	GDrawSetVisible(dv->ii.v,true);
	GDrawSetVisible(dv->dv,true);
	CVResize(cv);
	GDrawRequestExpose(cv->v,NULL,false);
	DVCreateRegs(dv);
	DVCreateStack(dv);
    } else {
	dv->scale = scale;
	DebuggerReset(dv->dc,cv->ft_pointsize,cv->ft_dpi,dbg_fpgm);
	if (( exc = DebuggerGetEContext(dv->dc))!=NULL )
	    DVFigureNewState(dv,exc);
    }
}
#endif

