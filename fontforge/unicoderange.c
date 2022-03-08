/* -*- coding: utf-8 -*- */
/* Copyright (C) 2006-2012 by George Williams */
/* 2012nov14, table updates, fixes added, Jose Da Silva */
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

#include <fontforge-config.h>

#include "fontforge.h"
#include "unicoderange.h"
#include "utype.h"

#include <stdlib.h>
#include <string.h>

static struct unicode_range nonunicode = { -1, -1, -1, 0, N_("Non-Unicode Glyphs") };
static struct unicode_range unassigned = { 0, 0x11ffff, 0, 0, N_("Unassigned Code Points") };

static int ucmp(const void *ri1_, const void *ri2_) {
    const struct rangeinfo *ri1 = ri1_, *ri2 = ri2_;
    return ri1->range->start == ri2->range->start ?
        ((int)ri2->range->end - (int)ri1->range->end) :
        ((int)ri1->range->start - (int)ri2->range->start);
}

struct rangeinfo *SFUnicodeRanges(SplineFont *sf, int include_empty) {
/* Find and return the Unicode range descriptions for these characters */
/* Return NULL if out of memory to hold rangeinfo[cnt]. */
    int cnt, num_planes, num_blocks;
    int i, gid;
    int32_t j;
    struct rangeinfo *ri;
    const struct unicode_range *planes, *blocks;

    planes = uniname_planes(&num_planes);
    blocks = uniname_blocks(&num_blocks);
    cnt = 2 + num_planes + num_blocks;

    ri = calloc(cnt + 1, sizeof(*ri));
    if (ri == NULL) {
        NoMoreMemMessage();
        return NULL;
    }

    for (i = 0; i < num_planes; ++i) {
        ri[i].range = &planes[i];
    }
    for (; i < num_planes + num_blocks; ++i) {
        ri[i].range = &blocks[i - num_planes];
    }
    ri[i++].range = &nonunicode;
    ri[i++].range = &unassigned;

    for (gid = 0; gid < sf->glyphcnt; ++gid) {
        if (sf->glyphs[gid] != NULL) {
            unichar_t ch = sf->glyphs[gid]->unicodeenc;
            const struct unicode_range *range = uniname_plane(ch);
            if (range != NULL) {
                ++ri[range - planes].cnt;
            }
            range = uniname_block(ch);
            if (range != NULL) {
                ++ri[(range - blocks) + num_planes].cnt;
            }

            if (ch > 0x10ffff) {
                /* non unicode glyphs (stylistic variations, etc.) */
                ++ri[num_planes + num_blocks].cnt;
            } else if (!isunicodepointassigned(ch)) {
                /* glyphs attached to code points which have not been assigned in */
                /*  the version of unicode I know about */
                ++ri[num_planes = num_blocks + 1].cnt;
            }
        }
    }

    if ( !include_empty ) {
	for ( i=j=0; i<cnt; ++i ) {
	    if ( ri[i].cnt!=0 ) {
		if ( i!=j )
		    ri[j] = ri[i];
		++j;
	    }
	}
	ri[j].range = NULL;
	cnt = j;
    }

    qsort(ri,cnt,sizeof(struct rangeinfo),ucmp);
return( ri );
}

const char *UnicodeRange(int32_t unienc) {
/* Return the best name that describes this Unicode value */
    const struct unicode_range* block;
    if ((block = uniname_block(unienc)) != NULL) {
        return block->name;
    }
    return "Unencoded Unicode";
}
