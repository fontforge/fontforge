#ifndef FONTFORGE_AUTOTRACE_H
#define FONTFORGE_AUTOTRACE_H

#include "baseviews.h"
#include "splinefont.h"

extern char **AutoTraceArgs(int ask);
extern char *ProgramExists(const char *prog, char *buffer);
extern const char *FindAutoTraceName(void);
extern const char *FindMFName(void);
extern SplineFont *SFFromMF(char *filename);
extern void FVAutoTrace(FontViewBase *fv, int ask);
extern void *GetAutoTraceArgs(void);
extern void MfArgsInit(void);
extern void _SCAutoTrace(SplineChar *sc, int layer, char **args);
extern void SetAutoTraceArgs(void *a);

#endif /* FONTFORGE_AUTOTRACE_H */
