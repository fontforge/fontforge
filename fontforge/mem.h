#ifndef FONTFORGE_MEM_H
#define FONTFORGE_MEM_H

#include "../inc/basics.h"

extern int32 memlong(uint8 *data, int len, int offset);
extern int memushort(uint8 *data, int len, int offset);
extern void memputshort(uint8 *data, int offset, uint16 val);

#endif /* FONTFORGE_MEM_H */
