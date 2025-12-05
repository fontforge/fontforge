/* Copyright (C) 2025 by Maxim Iorsh <iorsh@users@sourceforge.net>
 *
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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "gresource.h"

typedef struct gwindow* GWindow;

typedef struct {
    const char* name;
    uint32_t tag;
} LanguageRec;

typedef enum bitmaps_dlg_mode {
    bitmaps_dlg_avail,
    bitmaps_dlg_regen,
    bitmaps_dlg_remove
} BitmapsDlgMode;

int add_encoding_slots_dialog(GWindow parent, bool cid);

// Return comma-separated list of language tags, or NULL if the action was
// canceled. The caller is responsible to release the returned pointer.
char* language_list_dialog(GWindow parent, const LanguageRec* languages,
                           const char* initial_tags);

// Update the list of available bitmap pixel sizes and action.
// Arguments:
//  * c_sizes [input/output, may be reallocated inside] - NULL-terminated list
//    of bitmap pixel sizes
//  * bitmaps_only [input] - the font has bitmaps and no outlines
//  * has_current_char [input] - dialog called from Char View or similar context
//  * p_rasterize [output] - rasterze outlines to fill the newly created bitmaps
//  * p_scope [output, released by caller] - scope of change (all / selection /
//  current)
//  * return value - the user confirmed or dismissed the dialog
bool bitmap_strikes_dialog(GWindow parent, BitmapsDlgMode mode,
                           int32_t** c_sizes, bool bitmaps_only,
                           bool has_current_char, bool* p_rasterize,
                           char** p_scope);

void update_appearance();

#ifdef __cplusplus
}
#endif
