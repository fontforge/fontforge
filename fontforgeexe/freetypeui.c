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

#include "edgelist2.h"
#include "fffreetype.h"
#include "fontforgeui.h"
#include "gwidget.h"
#include "ustring.h"

#include <math.h>

/******************************************************************************/
/* ***************************** Debugger Stuff ***************************** */
/******************************************************************************/

#if FREETYPE_HAS_DEBUGGER

#include <pthread.h>
#include <tterrors.h>

typedef struct bpdata {
    int range;	/* tt_coderange_glyph, tt_coderange_font, tt_coderange_cvt */
    int ip;
} BpData;

struct debugger_context {
    FT_Library context;
    FTC *ftc;
    /* I use a thread because freetype doesn't return, it just has a callback */
    /*  on each instruction. In actuallity only one thread should be executable*/
    /*  at a time (either main, or child) */
    pthread_t thread;
    pthread_mutex_t parent_mutex, child_mutex;
    pthread_cond_t parent_cond, child_cond;
    unsigned int terminate: 1;		/* The thread has been started simply to clean itself up and die */
    unsigned int has_mutexes: 1;
    unsigned int has_thread: 1;
    unsigned int has_finished: 1;
    unsigned int debug_fpgm: 1;
    unsigned int multi_step: 1;
    unsigned int found_wp: 1;
    unsigned int found_wps: 1;
    unsigned int found_wps_uninit: 1;
    unsigned int found_wpc: 1;
    unsigned int initted_pts: 1;
    unsigned int is_bitmap: 1;
    int wp_ptindex, wp_cvtindex, wp_storeindex;
    real ptsizey, ptsizex;
    int dpi;
    TT_ExecContext exc;
    SplineChar *sc;
    int layer;
    BpData temp;
    BpData breaks[32];
    int bcnt;
    FT_Vector *oldpts;
    FT_Long *oldstore;
    uint8 *storetouched;
    int storeSize;
    FT_Long *oldcvt;
    FT_Long oldsval, oldcval;
    int n_points;
    uint8 *watch;		/* exc->pts.n_points */
    uint8 *watchstorage;	/* exc->storeSize, exc->storage[i] */
    uint8 *watchcvt;		/* exc->cvtSize, exc->cvt[i] */
    int uninit_index;
};

static int AtWp(struct debugger_context *dc, TT_ExecContext exc ) {
    int i, hit=false, h;

    dc->found_wp = false;
    if ( dc->watch!=NULL && dc->oldpts!=NULL ) {
	for ( i=0; i<exc->pts.n_points; ++i ) {
	    if ( dc->oldpts[i].x!=exc->pts.cur[i].x || dc->oldpts[i].y!=exc->pts.cur[i].y ) {
		dc->oldpts[i] = exc->pts.cur[i];
		if ( dc->watch[i] ) {
		    hit = true;
		    dc->wp_ptindex = i;
		}
	    }
	}
	dc->found_wp = hit;
    }
    if ( dc->found_wps_uninit )
	hit = true;
    dc->found_wps = false;
    if ( dc->watchstorage!=NULL && dc->storetouched!=NULL ) {
	h = false;
	for ( i=0; i<exc->storeSize; ++i ) {
	    if ( dc->storetouched[i]&2 ) {
		if ( dc->watchstorage[i] ) {
		    h = true;
		    dc->wp_storeindex = i;
		    dc->oldsval = dc->oldstore[i];
		}
		dc->storetouched[i]&=~2;
		dc->oldstore[i] = exc->storage[i];
	    }
	}
	dc->found_wps = h;
	hit |= h;
    }
    dc->found_wpc = false;
    if ( dc->watchcvt!=NULL && dc->oldcvt!=NULL ) {
	h = false;
	for ( i=0; i<exc->cvtSize; ++i ) {
	    if ( dc->oldcvt[i]!=exc->cvt[i] ) {
		if ( dc->watchcvt[i] ) {
		    h = true;
		    dc->wp_cvtindex = i;
		    dc->oldcval = dc->oldcvt[i];
		}
		dc->oldcvt[i] = exc->cvt[i];
	    }
	}
	dc->found_wpc = h;
	hit |= h;
    }
return( hit );
}

static int AtBp(struct debugger_context *dc, TT_ExecContext exc ) {
    int i;

    if ( dc->temp.range==exc->curRange && dc->temp.ip==exc->IP ) {
	dc->temp.range = tt_coderange_none;
return( true );
    }

    for ( i=0; i<dc->bcnt; ++i ) {
	if ( dc->breaks[i].range==exc->curRange && dc->breaks[i].ip==exc->IP )
return( true );
    }
return( false );
}

static void TestStorage( struct debugger_context *dc, TT_ExecContext exc) {
    int instr;

    if ( exc->code==NULL || exc->IP==exc->codeSize )
return;
    instr = exc->code[exc->IP];
    if ( instr==0x42 /* Write store */ && exc->top>=2 ) {
	int store_index = exc->stack[exc->top-2];
	if ( store_index>=0 && store_index<exc->storeSize )
	    dc->storetouched[store_index] = 3;	/* 2=>written this instr, 1=>ever written */
    } else if ( instr==0x43 /* Read Store */ && exc->top>=1 ) {
	int store_index = exc->stack[exc->top-1];
	if ( store_index>=0 && store_index<exc->storeSize &&
		!dc->storetouched[store_index] &&
		dc->watchstorage!=NULL && dc->watchstorage[store_index] ) {
	    dc->found_wps_uninit = true;
	    dc->uninit_index = store_index;
	}
    }
}

static struct debugger_context *massive_kludge;

static FT_Error PauseIns( TT_ExecContext exc ) {
    int ret;
    struct debugger_context *dc = massive_kludge;

    if ( dc->terminate )
return( TT_Err_Execution_Too_Long );		/* Some random error code, says we're probably in a infinite loop */
    dc->exc = exc;
    exc->grayscale = !dc->is_bitmap;		/* if we are in 'prep' or 'fpgm' freetype doesn't know this yet */

    /* Set up for watch points */
    if ( dc->oldpts==NULL && exc->pts.n_points!=0 ) {
	dc->oldpts = calloc(exc->pts.n_points,sizeof(FT_Vector));
	dc->n_points = exc->pts.n_points;
    }
    if ( dc->oldstore==NULL && exc->storeSize!=0 ) {
	dc->oldstore = calloc(exc->storeSize,sizeof(FT_Long));
	dc->storetouched = calloc(exc->storeSize,sizeof(uint8));
	dc->storeSize = exc->storeSize;
    }
    if ( dc->oldcvt==NULL && exc->cvtSize!=0 )
	dc->oldcvt = calloc(exc->cvtSize,sizeof(FT_Long));
    if ( !dc->initted_pts ) {
	AtWp(dc,exc);
	dc->found_wp = false;
	dc->found_wps = false;
	dc->found_wps_uninit = false;
	dc->found_wpc = false;
	dc->initted_pts = true;
    }

    if ( !dc->debug_fpgm && exc->curRange!=tt_coderange_glyph ) {
	exc->instruction_trap = 1;
	ret = 0;
	while ( exc->curRange!=tt_coderange_glyph ) {
	    TestStorage(dc,exc);
	    ret = TT_RunIns(exc);
	    if ( ret==TT_Err_Code_Overflow )
return( 0 );
	    if ( ret )
return( ret );
	}
return( ret );
    }

    pthread_mutex_lock(&dc->parent_mutex);
    pthread_cond_signal(&dc->parent_cond);
    pthread_mutex_unlock(&dc->parent_mutex);
    pthread_cond_wait(&dc->child_cond,&dc->child_mutex);
    if ( dc->terminate )
return( TT_Err_Execution_Too_Long );

    do {
	exc->instruction_trap = 1;
	if (exc->curRange==tt_coderange_glyph && exc->IP==exc->codeSize) {
	    ret = TT_Err_Code_Overflow;
    break;
	}
	TestStorage(dc,exc);
	ret = TT_RunIns(exc);
	if ( ret )
    break;
	/* Signal the parent if we are single stepping, or if we've reached a break-point */
	if ( AtWp(dc,exc) || !dc->multi_step || AtBp(dc,exc) ||
		(exc->curRange==tt_coderange_glyph && exc->IP==exc->codeSize)) {
	    if ( dc->found_wp ) {
		ff_post_notice(_("Hit Watch Point"),_("Point %d was moved by the previous instruction"),dc->wp_ptindex);
		dc->found_wp = false;
	    }
	    if ( dc->found_wps ) {
		ff_post_notice(_("Watched Store Change"),_("Storage %d was changed from %d (%.2f) to %d (%.2f) by the previous instruction"),
			dc->wp_storeindex, dc->oldsval, dc->oldsval/64.0,exc->storage[dc->wp_storeindex],exc->storage[dc->wp_storeindex]/64.0);
		dc->found_wps = false;
	    }
	    if ( dc->found_wps_uninit ) {
		ff_post_notice(_("Read of Uninitialized Store"),_("Storage %d has not been initialized, yet the previous instruction read it"),
			dc->uninit_index );
		dc->found_wps_uninit = false;
	    }
	    if ( dc->found_wpc ) {
		ff_post_notice(_("Watched Cvt Change"),_("Cvt %d was changed from %d (%.2f) to %d (%.2f) by the previous instruction"),
			dc->wp_cvtindex, dc->oldcval, dc->oldcval/64.0,exc->cvt[dc->wp_cvtindex],exc->cvt[dc->wp_cvtindex]/64.0);
		dc->found_wpc = false;
	    }
	    pthread_mutex_lock(&dc->parent_mutex);
	    pthread_cond_signal(&dc->parent_cond);
	    pthread_mutex_unlock(&dc->parent_mutex);
	    pthread_cond_wait(&dc->child_cond,&dc->child_mutex);
	}
    } while ( !dc->terminate );

    if ( ret==TT_Err_Code_Overflow )
	ret = 0;

    massive_kludge = dc;	/* We set this again in case we are in a composite character where I think we get called several times (and some other thread might have set it) */
    if ( dc->terminate )
return( TT_Err_Execution_Too_Long );

return( ret );
}

static void *StartChar(void *_dc) {
    struct debugger_context *dc = _dc;

    pthread_mutex_lock(&dc->child_mutex);

    massive_kludge = dc;
    if ( (dc->ftc = __FreeTypeFontContext(dc->context,dc->sc->parent,dc->sc,NULL,
	    dc->layer,ff_ttf, 0, NULL))==NULL )
 goto finish;
    if ( dc->storetouched!=NULL )
	memset(dc->storetouched,0,dc->storeSize);

    massive_kludge = dc;
    if ( FT_Set_Char_Size(dc->ftc->face,(int) (dc->ptsizex*64),(int) (dc->ptsizey*64), dc->dpi, dc->dpi))
 goto finish;

    massive_kludge = dc;
    FT_Load_Glyph(dc->ftc->face,dc->ftc->glyph_indeces[dc->sc->orig_pos],
	    dc->is_bitmap ? (FT_LOAD_NO_AUTOHINT|FT_LOAD_NO_BITMAP|FT_LOAD_TARGET_MONO) : (FT_LOAD_NO_AUTOHINT|FT_LOAD_NO_BITMAP));

 finish:
    dc->has_finished = true;
    dc->exc = NULL;
    pthread_mutex_lock(&dc->parent_mutex);
    pthread_cond_signal(&dc->parent_cond);	/* Wake up parent and get it to clean up after itself */
    pthread_mutex_unlock(&dc->parent_mutex);
    pthread_mutex_unlock(&dc->child_mutex);
return( NULL );
}

void DebuggerTerminate(struct debugger_context *dc) {
    if ( dc->has_thread ) {
	if ( !dc->has_finished ) {
	    dc->terminate = true;
	    pthread_mutex_lock(&dc->child_mutex);
	    pthread_cond_signal(&dc->child_cond);	/* Wake up child and get it to clean up after itself */
	    pthread_mutex_unlock(&dc->child_mutex);
	    pthread_mutex_unlock(&dc->parent_mutex);
	}
	pthread_join(dc->thread,NULL);
	dc->has_thread = false;
    }
    if ( dc->has_mutexes ) {
	pthread_cond_destroy(&dc->child_cond);
	pthread_cond_destroy(&dc->parent_cond);
	pthread_mutex_destroy(&dc->child_mutex);
	pthread_mutex_unlock(&dc->parent_mutex);	/* Is this actually needed? */
	pthread_mutex_destroy(&dc->parent_mutex);
    }
    if ( dc->ftc!=NULL )
	FreeTypeFreeContext(dc->ftc);
    if ( dc->context!=NULL )
	FT_Done_FreeType( dc->context );
    free(dc->watch);
    free(dc->oldpts);
    free(dc);
}

void DebuggerReset(struct debugger_context *dc,real ptsizey, real ptsizex,int dpi,int dbg_fpgm, int is_bitmap) {
    /* Kill off the old thread, and start up a new one working on the given */
    /*  pointsize and resolution */ /* I'm not prepared for errors here */
    /* Note that if we don't want to look at the fpgm/prep code (and we */
    /*  usually don't) then we must turn off the debug hook when they get run */

    if ( dc->has_thread ) {
	dc->terminate = true;
	pthread_mutex_lock(&dc->child_mutex);
	pthread_cond_signal(&dc->child_cond);	/* Wake up child and get it to clean up after itself */
	pthread_mutex_unlock(&dc->child_mutex);
	pthread_mutex_unlock(&dc->parent_mutex);

	pthread_join(dc->thread,NULL);
	dc->has_thread = false;
    }
    if ( dc->ftc!=NULL )
	FreeTypeFreeContext(dc->ftc);

    dc->debug_fpgm = dbg_fpgm;
    dc->ptsizey = ptsizey;
    dc->ptsizex = ptsizex;
    dc->dpi = dpi;
    dc->is_bitmap = is_bitmap;
    dc->terminate = dc->has_finished = false;
    dc->initted_pts = false;

    pthread_mutex_lock(&dc->parent_mutex);
    if ( pthread_create(&dc->thread,NULL,StartChar,(void *) dc)!=0 ) {
	DebuggerTerminate(dc);
return;
    }
    if ( dc->has_finished )
return;
    dc->has_thread = true;
    pthread_cond_wait(&dc->parent_cond,&dc->parent_mutex);
}

struct debugger_context *DebuggerCreate(SplineChar *sc,int layer, real ptsizey,real ptsizex,int dpi,int dbg_fpgm, int is_bitmap) {
    struct debugger_context *dc;

    if ( !hasFreeTypeDebugger())
return( NULL );

    dc = calloc(1,sizeof(struct debugger_context));
    dc->sc = sc;
    dc->layer = layer;
    dc->debug_fpgm = dbg_fpgm;
    dc->ptsizey = ptsizey;
    dc->ptsizex = ptsizex;
    dc->dpi = dpi;
    dc->is_bitmap = is_bitmap;
    if ( FT_Init_FreeType( &dc->context )) {
	free(dc);
return( NULL );
    }

#if FREETYPE_MINOR >= 5
    {
	int tt_version = TT_INTERPRETER_VERSION_35;

	if ( FT_Property_Set( dc->context,
			      "truetype",
			      "interpreter-version",
			      &tt_version )) {
	    free(dc);
return( NULL );
	}
    }
#endif

    FT_Set_Debug_Hook( dc->context,
		       FT_DEBUG_HOOK_TRUETYPE,
		       (FT_DebugHook_Func)PauseIns );

    pthread_mutex_init(&dc->parent_mutex,NULL); pthread_mutex_init(&dc->child_mutex,NULL);
    pthread_cond_init(&dc->parent_cond,NULL); pthread_cond_init(&dc->child_cond,NULL);
    dc->has_mutexes = true;

    pthread_mutex_lock(&dc->parent_mutex);
    if ( pthread_create(&dc->thread,NULL,StartChar,dc)!=0 ) {
	DebuggerTerminate( dc );
return( NULL );
    }
    dc->has_thread = true;
    pthread_cond_wait(&dc->parent_cond,&dc->parent_mutex);	/* Wait for the child to initialize itself (and stop) then we can look at its status */

return( dc );
}

void DebuggerGo(struct debugger_context *dc,enum debug_gotype dgt,DebugView *dv) {
    int opcode;

    if ( !dc->has_thread || dc->has_finished || dc->exc==NULL ) {
	FreeType_FreeRaster(dv->cv->raster); dv->cv->raster = NULL;
	DebuggerReset(dc,dc->ptsizey,dc->ptsizex,dc->dpi,dc->debug_fpgm,dc->is_bitmap);
    } else {
	switch ( dgt ) {
	  case dgt_continue:
	    dc->multi_step = true;
	  break;
	  case dgt_stepout:
	    dc->multi_step = true;
	    if ( dc->exc->callTop>0 ) {
		dc->temp.range = dc->exc->callStack[dc->exc->callTop-1].Caller_Range;
		dc->temp.ip = dc->exc->callStack[dc->exc->callTop-1].Caller_IP;
	    }
	  break;
	  case dgt_next:
	    opcode = dc->exc->code[dc->exc->IP];
	    /* I've decided that IDEFs will get stepped into */
	    if ( opcode==0x2b /* call */ || opcode==0x2a /* loopcall */ ) {
		dc->temp.range = dc->exc->curRange;
		dc->temp.ip = dc->exc->IP+1;
		dc->multi_step = true;
	    } else
		dc->multi_step = false;
	  break;
	  default:
	  case dgt_step:
	    dc->multi_step = false;
	  break;
	}
	pthread_mutex_lock(&dc->child_mutex);
	pthread_cond_signal(&dc->child_cond);	/* Wake up child and get it to clean up after itself */
	pthread_mutex_unlock(&dc->child_mutex);
	pthread_cond_wait(&dc->parent_cond,&dc->parent_mutex);	/* Wait for the child to initialize itself (and stop) then we can look at its status */
    }
}

struct TT_ExecContextRec_ *DebuggerGetEContext(struct debugger_context *dc) {
return( dc->exc );
}

int DebuggerBpCheck(struct debugger_context *dc,int range,int ip) {
    int i;

    for ( i=0; i<dc->bcnt; ++i ) {
	if ( dc->breaks[i].range==range && dc->breaks[i].ip==ip )
return( true );
    }
return( false );
}

void DebuggerToggleBp(struct debugger_context *dc,int range,int ip) {
    int i;

    /* If the address has a bp, then remove it */
    for ( i=0; i<dc->bcnt; ++i ) {
	if ( dc->breaks[i].range==range && dc->breaks[i].ip==ip ) {
	    ++i;
	    while ( i<dc->bcnt ) {
		dc->breaks[i-1].range = dc->breaks[i].range;
		dc->breaks[i-1].ip = dc->breaks[i].ip;
		++i;
	    }
	    --dc->bcnt;
return;
	}
    }
    /* Else add it */
    if ( dc->bcnt>=sizeof(dc->breaks)/sizeof(dc->breaks[0]) ) {
	ff_post_error(_("Too Many Breakpoints"),_("Too Many Breakpoints"));
return;
    }
    i = dc->bcnt++;
    dc->breaks[i].range = range;
    dc->breaks[i].ip = ip;
}

void DebuggerSetWatches(struct debugger_context *dc,int n, uint8 *w) {
    free(dc->watch); dc->watch=NULL;
    if ( n!=dc->n_points ) IError("Bad watchpoint count");
    else {
	dc->watch = w;
	if ( dc->exc ) {
	    AtWp(dc,dc->exc);
	    dc->found_wp = false;
	    dc->found_wpc = false;
	    dc->found_wps = false;
	    dc->found_wps_uninit = false;
	}
    }
}

uint8 *DebuggerGetWatches(struct debugger_context *dc, int *n) {
    *n = dc->n_points;
return( dc->watch );
}

void DebuggerSetWatchStores(struct debugger_context *dc,int n, uint8 *w) {
    free(dc->watchstorage); dc->watchstorage=NULL;
    if ( n!=dc->exc->storeSize ) IError("Bad watchpoint count");
    else {
	dc->watchstorage = w;
	if ( dc->exc ) {
	    AtWp(dc,dc->exc);
	    dc->found_wp = false;
	    dc->found_wpc = false;
	    dc->found_wps = false;
	    dc->found_wps_uninit = false;
	}
    }
}

uint8 *DebuggerGetWatchStores(struct debugger_context *dc, int *n) {
    *n = dc->exc->storeSize;
return( dc->watchstorage );
}

int DebuggerIsStorageSet(struct debugger_context *dc, int index) {
    if ( dc->storetouched==NULL )
return( false );
return( dc->storetouched[index]&1 );
}

void DebuggerSetWatchCvts(struct debugger_context *dc,int n, uint8 *w) {
    free(dc->watchcvt); dc->watchcvt=NULL;
    if ( n!=dc->exc->cvtSize ) IError("Bad watchpoint count");
    else {
	dc->watchcvt = w;
	if ( dc->exc ) {
	    AtWp(dc,dc->exc);
	    dc->found_wp = false;
	    dc->found_wpc = false;
	    dc->found_wps = false;
	    dc->found_wps_uninit = false;
	}
    }
}

uint8 *DebuggerGetWatchCvts(struct debugger_context *dc, int *n) {
    *n = dc->exc->cvtSize;
return( dc->watchcvt );
}

int DebuggingFpgm(struct debugger_context *dc) {
return( dc->debug_fpgm );
}

struct freetype_raster *DebuggerCurrentRaster(TT_ExecContext exc,int depth) {
    FT_Outline outline;
    FT_Bitmap bitmap;
    int i, err, j, k, first, xoff, yoff;
    IBounds b;
    struct freetype_raster *ret;

    outline.n_contours = exc->pts.n_contours;
    outline.tags = (char *) exc->pts.tags;
    outline.contours = (short *) exc->pts.contours;
    /* Rasterizer gets unhappy if we give it the phantom points */
    if ( outline.n_contours==0 )
	outline.n_points = 0;
    else
	outline.n_points = /*exc->pts.n_points*/  outline.contours[outline.n_contours - 1] + 1;
    outline.points = exc->pts.cur;
    outline.flags = 0;
    switch ( exc->GS.scan_type ) {
      /* Taken, at Werner's suggestion, from the freetype sources: ttgload.c:1970 */
      case 0: /* simple drop-outs including stubs */
	outline.flags |= FT_OUTLINE_INCLUDE_STUBS;
      break;
      case 1: /* simple drop-outs excluding stubs */
	/* nothing; it's the default rendering mode */
      break;
      case 4: /* smart drop-outs including stubs */
	outline.flags |= FT_OUTLINE_SMART_DROPOUTS |
				FT_OUTLINE_INCLUDE_STUBS;
      break;
      case 5: /* smart drop-outs excluding stubs  */
	outline.flags |= FT_OUTLINE_SMART_DROPOUTS;
      break;

      default: /* no drop-out control */
	outline.flags |= FT_OUTLINE_IGNORE_DROPOUTS;
      break;
    }

    if ( exc->metrics.y_ppem < 24 )
       outline.flags |= FT_OUTLINE_HIGH_PRECISION;

    first = true;
    for ( k=0; k<outline.n_contours; ++k ) {
	if ( outline.contours[k] - (k==0?-1:outline.contours[k-1])>1 ) {
	    /* Single point contours are used for things like point matching */
	    /*  for anchor points, etc. and do not contribute to the bounding */
	    /*  box */
	    i = (k==0?0:(outline.contours[k-1]+1));
	    if ( first ) {
		b.minx = b.maxx = outline.points[i].x;
		b.miny = b.maxy = outline.points[i++].y;
		first = false;
	    }
	    for ( ; i<=outline.contours[k]; ++i ) {
		if ( outline.points[i].x>b.maxx ) b.maxx = outline.points[i].x;
		if ( outline.points[i].x<b.minx ) b.minx = outline.points[i].x;
		if ( outline.points[i].y>b.maxy ) b.maxy = outline.points[i].y;
		if ( outline.points[i].y<b.miny ) b.miny = outline.points[i].y;
	    }
	}
    }
    if ( first )
	memset(&b,0,sizeof(b));

    memset(&bitmap,0,sizeof(bitmap));
    bitmap.rows = (((int) (ceil(b.maxy/64.0)-floor(b.miny/64.0)))) +1;
    bitmap.width = (((int) (ceil(b.maxx/64.0)-floor(b.minx/64.0)))) +1;

    xoff = 64*floor(b.minx/64.0);
    yoff = 64*floor(b.miny/64.0);
    for ( i=0; i<outline.n_points; ++i ) {
	outline.points[i].x -= xoff;
	outline.points[i].y -= yoff;
    }

    if ( depth==8 ) {
	bitmap.pitch = bitmap.width;
	bitmap.num_grays = 256;
	bitmap.pixel_mode = ft_pixel_mode_grays;
    } else {
	bitmap.pitch = (bitmap.width+7)>>3;
	bitmap.num_grays = 0;
	bitmap.pixel_mode = ft_pixel_mode_mono;
    }
    bitmap.buffer = calloc(bitmap.pitch*bitmap.rows,sizeof(uint8));

    err = (FT_Outline_Get_Bitmap)(ff_ft_context,&outline,&bitmap);

    for ( i=0; i<outline.n_points; ++i ) {
	outline.points[i].x += xoff;
	outline.points[i].y += yoff;
    }

    ret = malloc(sizeof(struct freetype_raster));
    /* I'm not sure why I need these, but it seems I do */
    if ( depth==8 ) {
	ret->as = floor(b.miny/64.0) + bitmap.rows;
	ret->lb = floor(b.minx/64.0);
    } else {
	for ( k=0; k<bitmap.rows; ++k ) {
	    for ( j=bitmap.pitch-1; j>=0 && bitmap.buffer[k*bitmap.pitch+j]==0; --j );
	    if ( j!=-1 )
	break;
	}
	b.maxy += k<<6;
	if ( depth==8 ) {
	    for ( j=0; j<bitmap.pitch; ++j ) {
		for ( k=(((int) (b.maxy-b.miny))>>6)-1; k>=0; --k ) {
		    if ( bitmap.buffer[k*bitmap.pitch+j]!=0 )
		break;
		}
		if ( k!=-1 )
	    break;
	    }
	} else {
	    for ( j=0; j<bitmap.pitch; ++j ) {
		for ( k=(((int) (b.maxy-b.miny))>>6)-1; k>=0; --k ) {
		    if ( bitmap.buffer[k*bitmap.pitch+(j>>3)]&(0x80>>(j&7)) )
		break;
		}
		if ( k!=-1 )
	    break;
	    }
	}
	b.minx -= j*64;
	ret->as = rint(b.maxy/64.0);
	ret->lb = rint(b.minx/64.0);
    }
    ret->rows = bitmap.rows;
    ret->cols = bitmap.width;
    ret->bytes_per_row = bitmap.pitch;
    ret->num_greys = bitmap.num_grays;
    ret->bitmap = bitmap.buffer;
return( ret );
}
    
#else /* FIXME: Don't build this stuff if it's not being used, it just makes the compiler emit lots of warnings */
struct debugger_context;

void DebuggerTerminate(struct debugger_context *dc) {
}

void DebuggerReset(struct debugger_context *dc,real ptsizey, real ptsizex,int dpi,int dbg_fpgm, int is_bitmap) {
}

struct debugger_context *DebuggerCreate(SplineChar *sc,int layer,real pointsizey, real pointsizex,int dpi, int dbg_fpgm, int is_bitmap) {
return( NULL );
}

void DebuggerGo(struct debugger_context *dc,enum debug_gotype go,DebugView *dv) {
}

struct TT_ExecContextRec_ *DebuggerGetEContext(struct debugger_context *dc) {
return( NULL );
}

void DebuggerSetWatches(struct debugger_context *dc,int n, uint8 *w) {
}

uint8 *DebuggerGetWatches(struct debugger_context *dc, int *n) {
    *n = 0;
return( NULL );
}

void DebuggerSetWatchStores(struct debugger_context *dc,int n, uint8 *w) {
}

uint8 *DebuggerGetWatchStores(struct debugger_context *dc, int *n) {
    *n = 0;
return( NULL );
}

int DebuggerIsStorageSet(struct debugger_context *dc, int index) {
return( false );
}

void DebuggerSetWatchCvts(struct debugger_context *dc,int n, uint8 *w) {
}

uint8 *DebuggerGetWatchCvts(struct debugger_context *dc, int *n) {
    *n = 0;
return( NULL );
}

int DebuggingFpgm(struct debugger_context *dc) {
return( false );
}
#endif	/* FREETYPE_HAS_DEBUGGER */
