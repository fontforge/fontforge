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

enum ProblemRecType { prob_bool, prob_int, prob_double };

typedef struct {
    short cid;
    const char* label;
    const char* tooltip;
    bool active;
    enum ProblemRecType type;
    short parent_cid;
    union {
        int ival;
        double dval;
    } value;
    bool disabled;
} ProblemRec;
#define PROBLEM_REC_EMPTY \
    { 0, NULL, NULL, false, prob_bool, 0, {0}, false }

typedef struct {
    const char* label;
    ProblemRec* records;
} ProblemTab;
#define PROBLEM_TAB_EMPTY \
    { NULL, NULL }

int add_encoding_slots_dialog(GWindow parent, bool cid);

// Return comma-separated list of language tags, or NULL if the action was
// canceled. The caller is responsible to release the returned pointer.
char* language_list_dialog(GWindow parent, const LanguageRec* languages,
                           const char* initial_tags);

/* This function updates pr_tabs in-place to preserve the state of the dialog
   between invocations.

   Return value: true, if any problem record was selected. The selected records
                 are marked as active in pr_tabs. */
bool find_problems_dialog(GWindow parent, ProblemTab* pr_tabs, double* near);

void update_appearance();

#ifdef __cplusplus
}
#endif
