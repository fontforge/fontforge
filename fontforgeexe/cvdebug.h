#ifndef FONTFORGE_FONTFORGEEXE_CVDEBUG_H
#define FONTFORGE_FONTFORGEEXE_CVDEBUG_H

extern int CVXPos(DebugView *dv,int offset,int width);
extern int DVChar(DebugView *dv, GEvent *event);
extern void CVDebugFree(DebugView *dv);
extern void CVDebugPointPopup(CharView *cv);
extern void CVDebugReInit(CharView *cv,int restart_debug,int dbg_fpgm);

#endif /* FONTFORGE_FONTFORGEEXE_CVDEBUG_H */
