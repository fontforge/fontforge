#ifndef FONTFORGE_SPIRO_H
#define FONTFORGE_SPIRO_H

#include "splinefont.h"

#define SPIRO_SELECTED(cp)      ((cp)->ty&0x80)
#define SPIRO_DESELECT(cp)      ((cp)->ty&=~0x80)
#define SPIRO_SELECT(cp)        ((cp)->ty|=0x80)
#define SPIRO_SPL_OPEN(spl)     ((spl)->spiro_cnt>1 && ((spl)->spiros[0].ty&0x7f)==SPIRO_OPEN_CONTOUR)

#define SPIRO_NEXT_CONSTRAINT   SPIRO_RIGHT    /* The curve is on the next side of the constraint point */
#define SPIRO_PREV_CONSTRAINT   SPIRO_LEFT     /* The curve is on the prev side of the constraint point */

extern char *libspiro_version(void);
extern int hasspiro(void);
extern spiro_cp *SpiroCPCopy(spiro_cp *spiros, uint16_t *_cnt);
extern spiro_cp *SplineSet2SpiroCP(SplineSet *ss, uint16_t *cnt);
extern SplineSet *SpiroCP2SplineSet(spiro_cp *spiros);
extern void SSRegenerateFromSpiros(SplineSet *spl);

#endif /* FONTFORGE_SPIRO_H */
