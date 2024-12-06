/* Copyright 2023 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include <stdint.h>

typedef struct _GtkWidget GtkWidget;
typedef struct fontview_context FVContext;

#ifdef __cplusplus
extern "C" {
#endif

// Create GTK Font View window.
// Return value:
//    pointer to ff::views::FontView object, opaque to C code
void* create_font_view(FVContext** p_fv_context, int width, int height);

// Set views::FontView title and taskbar title [unsupported]
void gtk_set_title(void* fv_opaque, char* window_title, char* taskbar_title);

GtkWidget* get_drawing_widget_c(void* fv_opaque);

void fv_set_scroller_position(void* fv_opaque, int32_t position);

void fv_set_scroller_bounds(void* fv_opaque, int32_t sb_min, int32_t sb_max,
                            int32_t sb_pagesize);

#ifdef __cplusplus
}
#endif
