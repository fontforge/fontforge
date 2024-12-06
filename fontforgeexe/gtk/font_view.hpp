/* Copyright 2024 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include <string>
#include <gtkmm.h>

#include "c_context.h"
#include "char_grid.hpp"

namespace ff::views {

class FontView {
 public:
    FontView(std::shared_ptr<FVContext> context, int width, int height);

    void set_title(const std::string& window_title,
                   const std::string& taskbar_title) {
        window.set_title(window_title);
    }

    GtkWidget* get_drawing_widget_c() {
        return char_grid.get_drawing_widget_c();
    }

    void set_scroller_position(int32_t position) {
        char_grid.set_scroller_position(position);
    }

    void set_scroller_bounds(int32_t sb_min, int32_t sb_max,
                             int32_t sb_pagesize) {
        char_grid.set_scroller_bounds(sb_min, sb_max, sb_pagesize);
    }

    void set_character_info(const std::string& info) {
        char_grid.set_character_info(info);
    }

    void resize_drawing_area(int width, int height) {
        char_grid.resize_drawing_area(width, height);
    }

 private:
    std::shared_ptr<FVContext> fv_context;

    Gtk::Window window;
    CharGrid char_grid;
};

}  // namespace ff::views