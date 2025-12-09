/* Copyright (C) 2000-2012 by George Williams */
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

#ifndef FONTFORGE_BDFFONT_H
#define FONTFORGE_BDFFONT_H

#include <stdint.h>
#include "splinechar.h"

enum property_type {
    prt_string,
    prt_atom,
    prt_int,
    prt_uint,
    prt_property = 0x10
};

typedef struct bdfprops {
    char* name; /* These include both properties (like SLANT) and non-properties
                   (like FONT) */
    int type;
    union {
        char* str;
        char* atom;
        int val;
    } u;
} BDFProperties;

typedef struct bdffont {
    struct splinefont* sf;
    int glyphcnt, glyphmax; /* used & allocated sizes of glyphs array */
    BDFChar** glyphs;       /* an array of charcnt entries */
    int16_t pixelsize;
    int16_t ascent, descent;
    int16_t layer; /* for piecemeal fonts */
    unsigned int piecemeal : 1;
    unsigned int bbsized : 1;
    unsigned int ticked : 1;
    unsigned int unhinted_freetype : 1;
    unsigned int recontext_freetype : 1;
    struct bdffont* next;
    struct clut* clut;
    char* foundry;
    int res;
    void* freetype_context;
    uint16_t truesize; /* for bbsized fonts */
    int16_t prop_cnt;
    int16_t prop_max; /* only used within bdfinfo dlg */
    BDFProperties* props;
    uint16_t ptsize, dpi; /* for piecemeal fonts */
} BDFFont;

extern int BDFDepth(BDFFont* bdf);

#endif /* FONTFORGE_BDFFONT_H */
