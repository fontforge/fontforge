/* Copyright (C) 2005-2012 by George Williams */
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

#ifndef FONTFORGE_MEM_H
#define FONTFORGE_MEM_H

#include <fontforge-config.h>

#include "basics.h"

// FIXME: move from splinefont.h to a separate header
#ifdef FONTFORGE_CONFIG_USE_DOUBLE
# define real           double
# define bigreal        double
#else
# define real           float
# define bigreal        double
#endif

extern int32_t memlong(uint8_t *data, int len, int offset);
extern int memushort(uint8_t *data, int len, int offset);
extern void memputshort(uint8_t *data, int offset, uint16_t val);

extern int getushort(FILE *ttf);
extern int get3byte(FILE *ttf);
extern int32_t getlong(FILE *ttf);
extern real getfixed(FILE *ttf);
extern real get2dot14(FILE *ttf);

#endif /* FONTFORGE_MEM_H */
