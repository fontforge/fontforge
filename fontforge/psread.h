#ifndef FONTFORGE_PSREAD_H
#define FONTFORGE_PSREAD_H

#include <fontforge-config.h>
#include <stdio.h>

#include "splinefont.h"
#include "sd.h"

extern Encoding *PSSlurpEncodings(FILE *file);
extern Entity *EntityInterpretPS(FILE *ps, int *width);
extern int EvaluatePS(char *str, real *stack, int size);
extern int MatIsIdentity(real transform[6]);
extern int UnblendedCompare(real u1[MmMax], real u2[MmMax], int cnt);
extern SplineChar *PSCharStringToSplines(uint8 *type1, int len, struct pscontext *context, struct pschars *subrs, struct pschars *gsubrs, const char *name);
extern SplinePointList *SplinePointListInterpretPS(FILE *ps, int flags, int is_stroked, int *width);
extern SplinePointList *SplinesFromEntities(Entity *ent, int *flags, int is_stroked);
extern SplinePointList *SplinesFromEntityChar(EntityChar *ec, int *flags, int is_stroked);
extern void EntityDefaultStrokeFill(Entity *ent);
extern void MatInverse(real into[6], real orig[6]);
extern void MatMultiply(real m1[6], real m2[6], real to[6]);
extern void PSFontInterpretPS(FILE *ps, struct charprocs *cp, char **encoding);
extern void SFSetLayerWidthsStroked(SplineFont *sf, real strokewidth);
extern void SFSplinesFromLayers(SplineFont *sf, int tostroke);

#endif /* FONTFORGE_PSREAD_H */
