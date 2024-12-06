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

#include "font_view_shim.hpp"

#include "c_context.h"
#include "font_view.hpp"

#include <gtkmm.h>

void* create_font_view(FVContext** p_fv_context, int width, int height) {
    static auto app = Gtk::Application::create("org.fontforge");

    // Take ownership of *p_fv_context
    std::shared_ptr<FVContext> context(*p_fv_context, &free);
    ff::views::FontView* font_view =
        new ff::views::FontView(context, width, height);
    *p_fv_context = NULL;
    return font_view;
}

void gtk_set_title(void* fv_opaque, char* window_title, char* taskbar_title) {
    ff::views::FontView* font_view =
        static_cast<ff::views::FontView*>(fv_opaque);

    if (font_view != nullptr) {
        font_view->set_title(window_title, taskbar_title);
    }
}

GtkWidget* get_drawing_widget_c(void* fv_opaque) {
    ff::views::FontView* font_view =
        static_cast<ff::views::FontView*>(fv_opaque);
    return font_view->get_drawing_widget_c();
}

void fv_set_scroller_position(void* fv_opaque, int32_t position) {
    ff::views::FontView* font_view =
        static_cast<ff::views::FontView*>(fv_opaque);
    font_view->set_scroller_position(position);
}

void fv_set_scroller_bounds(void* fv_opaque, int32_t sb_min, int32_t sb_max,
                            int32_t sb_pagesize) {
    ff::views::FontView* font_view =
        static_cast<ff::views::FontView*>(fv_opaque);
    font_view->set_scroller_bounds(sb_min, sb_max, sb_pagesize);
}

void fv_set_character_info(void* fv_opaque, char* info) {
    ff::views::FontView* font_view =
        static_cast<ff::views::FontView*>(fv_opaque);
    font_view->set_character_info(info);
}
