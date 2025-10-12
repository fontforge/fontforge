/* Copyright 2023 Joey Sabey <github.com/Omnikron13>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
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

int add_encoding_slots_dialog(GWindow parent, bool cid);

// Return comma-separated list of language tags, or NULL if the action was
// canceled. The caller is responsible to release the retirned pointer.
char* language_list_dialog(GWindow parent, LanguageRec* languages);

void update_appearance();

#ifdef __cplusplus
}
#endif
