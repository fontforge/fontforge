#ifndef FONTFORGE_PSREAD_H
#define FONTFORGE_PSREAD_H

#include <fontforge-config.h>

#include "sd.h"
#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Encoding *PSSlurpEncodings(FILE *file);
extern Entity *EntityInterpretPS(FILE *ps, int *width);
extern int EvaluatePS(char *str, real *stack, int size);
extern int MatIsIdentity(real transform[6]);
extern int UnblendedCompare(real u1[MmMax], real u2[MmMax], int cnt);
extern SplineChar *PSCharStringToSplines(uint8_t *type1, int len, struct pscontext *context, struct pschars *subrs, struct pschars *gsubrs, const char *name);
extern SplinePointList *SplinePointListInterpretPS(FILE *ps, ImportParams *ip, int is_stroked, int *width);
extern SplinePointList *SplinesFromEntities(Entity *ent, ImportParams *ip, int is_stroked);
extern SplinePointList *SplinesFromEntityChar(EntityChar *ec, ImportParams *ip, int is_stroked);
extern void EntityDefaultStrokeFill(Entity *ent);
extern void MatInverse(real into[6], real orig[6]);
extern void MatMultiply(real m1[6], real m2[6], real to[6]);
extern void PSFontInterpretPS(FILE *ps, struct charprocs *cp, char **encoding);
extern void SFSetLayerWidthsStroked(SplineFont *sf, real strokewidth);
extern void SFSplinesFromLayers(SplineFont *sf, int tostroke);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_PSREAD_H */
