#ifndef FONTFORGE_MEM_H
#define FONTFORGE_MEM_H

#include "../inc/basics.h"
#include <inc/fontforge-config.h>

// FIXME: move from splinefont.h to a separate header
#ifdef FONTFORGE_CONFIG_USE_DOUBLE
# define real           double
# define bigreal        double
#else
# define real           float
# define bigreal        double
#endif

extern int32 memlong(uint8 *data, int len, int offset);
extern int memushort(uint8 *data, int len, int offset);
extern void memputshort(uint8 *data, int offset, uint16 val);

extern int getushort(FILE *ttf);
extern int get3byte(FILE *ttf);
extern int32 getlong(FILE *ttf);
extern real getfixed(FILE *ttf);
extern real get2dot14(FILE *ttf);

#endif /* FONTFORGE_MEM_H */
