#ifndef FONTFORGE_FONTFORGEEXE_FREETYPEUI_H
#define FONTFORGE_FONTFORGEEXE_FREETYPEUI_H

#include <fontforge-config.h>

enum debug_gotype { dgt_continue, dgt_step, dgt_next, dgt_stepout };

#if FREETYPE_HAS_DEBUGGER

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

extern struct debugger_context *DebuggerCreate(SplineChar *sc, int layer, real pointsizey, real pointsizex, int dpi, int dbg_fpgm, int is_bitmap);

extern int DebuggerBpCheck(struct debugger_context *dc, int range, int ip);
extern int DebuggerIsStorageSet(struct debugger_context *dc, int index);
extern int DebuggingFpgm(struct debugger_context *dc);
extern struct debugger_context *DebuggerCreate(SplineChar *sc, int layer, real pointsizey, real pointsizex, int dpi, int dbg_fpgm, int is_bitmap);
extern struct freetype_raster *DebuggerCurrentRaster(TT_ExecContext exc, int depth);
extern struct TT_ExecContextRec_ *DebuggerGetEContext(struct debugger_context *dc);
extern uint8 *DebuggerGetWatchCvts(struct debugger_context *dc, int *n);
extern uint8 *DebuggerGetWatches(struct debugger_context *dc, int *n);
extern uint8 *DebuggerGetWatchStores(struct debugger_context *dc, int *n);
extern void DebuggerGo(struct debugger_context *dc, enum debug_gotype dgt, DebugView *dv);
extern void DebuggerReset(struct debugger_context *dc, real ptsizey, real ptsizex, int dpi, int dbg_fpgm, int is_bitmap);
extern void DebuggerSetWatchCvts(struct debugger_context *dc, int n, uint8 *w);
extern void DebuggerSetWatches(struct debugger_context *dc, int n, uint8 *w);
extern void DebuggerSetWatchStores(struct debugger_context *dc, int n, uint8 *w);
extern void DebuggerTerminate(struct debugger_context *dc);
extern void DebuggerToggleBp(struct debugger_context *dc, int range, int ip);

#endif /* FREETYPE_HAS_DEBUGGER */

#endif /* FONTFORGE_FONTFORGEEXE_FREETYPEUI_H */
