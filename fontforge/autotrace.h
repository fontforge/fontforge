#ifndef FONTFORGE_AUTOTRACE_H
#define FONTFORGE_AUTOTRACE_H

#include "baseviews.h"
#include "splinefont.h"

extern char **AutoTraceArgs(int ask);
extern char *ProgramExists(const char *prog, char *buffer);
extern int FindAutoTrace(char *path, size_t bufsiz);
extern const char *FindMFName(void);
extern SplineFont *SFFromMF(char *filename);
extern void FVAutoTrace(FontViewBase *fv, int ask);
extern void *GetAutoTraceArgs(void);
extern void MfArgsInit(void);
extern void SetAutoTraceArgs(void *a);

#endif /* FONTFORGE_AUTOTRACE_H */
