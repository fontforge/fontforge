/* Copyright (C) 2021 by Skef Iterum */
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

#ifndef FONTFORGE_MULTIDIALOG_H
#define FONTFORGE_MULTIDIALOG_H

#include <fontforge-config.h>

enum multi_dlg_question_type { mdq_openpath, mdq_savepath, mdq_string, mdq_choice };

/* For _open, _save, and _askstr "in" is the default value (if any) and
 * out is the user-supplied value.
 *
 * For _choice "out" is kept NULL and is_checked indicates the user-supplied
 * value. If "tag" is present it is used to represent the answer value in place
 * of "in".
 */
struct multi_dlg_question;

typedef struct multi_dlg_answer {
    void *tag;
    unsigned int is_default: 1;
    unsigned int is_checked: 1;
    char *name;
    struct multi_dlg_question *question;
} MultiDlgAnswer;

typedef struct multi_dlg_question {
    void *tag;
    enum multi_dlg_question_type type;
    int answer_len;
    unsigned int multiple: 1;
    unsigned int checks: 1;
    unsigned int align: 1;
    char *label, *dflt, *filter, *str_answer;
    MultiDlgAnswer *answers;
} MultiDlgQuestion;

typedef struct multi_dlg_category {
    int len;
    char *label;
    MultiDlgQuestion *questions;
} MultiDlgCategory;

typedef struct multi_dlg_spec {
    int len;
    MultiDlgCategory *categories;
} MultiDlgSpec;

#ifdef __cplusplus
extern "C" {
#endif
extern void multiDlgFree(MultiDlgSpec *dlg, int do_top);
#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_MULTIDIALOG_H */
