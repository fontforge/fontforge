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

#include "application.hpp"
#include "c_context.h"
#include "common_menus.hpp"
#include "font_view.hpp"
#include "kerning_format.hpp"
#include "select_glyphs.hpp"

#include <gtkmm.h>

using ff::dlg::KerningFormat;
using ff::dlg::SelectGlyphs;
using ff::views::CharGrid;
using ff::views::ICharGridContainter;

void* create_font_view(FVContext** p_fv_context, int width, int height) {
    // TODO(myoresh): move to main() once it becomes GTK-aware.
    ff::app::GtkApp();

    // Take ownership of *p_fv_context
    std::shared_ptr<FVContext> context(*p_fv_context, &free);
    ff::views::FontView* font_view =
        new ff::views::FontView(context, width, height);
    *p_fv_context = NULL;
    return dynamic_cast<ICharGridContainter*>(font_view);
}

void* get_char_grid_widget(void* cg_dlg, int char_grid_index) {
    auto cg_container_dlg = static_cast<ICharGridContainter*>(cg_dlg);
    return &(cg_container_dlg->get_char_grid(char_grid_index != 0));
}

void cg_set_dlg_title(void* cg_opaque, char* window_title,
                      char* taskbar_title) {
    auto char_grid = static_cast<CharGrid*>(cg_opaque);
    Gtk::Widget& widget = char_grid->get_top_widget();
    Glib::RefPtr<Gdk::Window> window = widget.get_window();

    if (window) {
        window->set_title(window_title);
    }
}

GtkWidget* cg_get_drawing_widget_c(void* cg_opaque) {
    auto char_grid = static_cast<CharGrid*>(cg_opaque);
    return char_grid->get_drawing_widget_c();
}

void cg_set_scroller_position(void* cg_opaque, int32_t position) {
    auto char_grid = static_cast<CharGrid*>(cg_opaque);
    char_grid->set_scroller_position(position);
}

void cg_set_scroller_bounds(void* cg_opaque, int32_t sb_min, int32_t sb_max,
                            int32_t sb_pagesize) {
    auto char_grid = static_cast<CharGrid*>(cg_opaque);
    char_grid->set_scroller_bounds(sb_min, sb_max, sb_pagesize);
}

void cg_set_character_info(void* cg_opaque, char* info) {
    auto char_grid = static_cast<CharGrid*>(cg_opaque);
    char_grid->set_character_info(info);
}

void cg_resize_window(void* cg_opaque, int width, int height) {
    auto char_grid = static_cast<CharGrid*>(cg_opaque);
    char_grid->resize_drawing_area(width, height);
}

void cg_raise_window(void* cg_opaque) {
    auto char_grid = static_cast<CharGrid*>(cg_opaque);
    char_grid->raise_window();
}

void* create_select_glyphs_dlg(FVContext** p_fv_context, int width,
                               int height) {
    // Take ownership of *p_fv_context
    std::shared_ptr<FVContext> context(*p_fv_context, &free);
    SelectGlyphs* sel_glyphs_dlg = new SelectGlyphs(context, width, height);
    *p_fv_context = NULL;

    // To prevent issues with multiple inheritance, the final static void*
    // casting should occur only to/from ICharGridContainter*
    return dynamic_cast<ICharGridContainter*>(sel_glyphs_dlg);
}

bool run_select_glyphs_dlg(void** sg_opaque) {
    // To prevent issues with multiple inheritance, the final static void*
    // casting should occur only to/from ICharGridContainter*
    auto sel_glyphs_dlg = dynamic_cast<SelectGlyphs*>(
        static_cast<ICharGridContainter*>(*sg_opaque));
    Gtk::ResponseType response = sel_glyphs_dlg->run();

    // Destroy dialog and reset pointer
    delete sel_glyphs_dlg;
    *sg_opaque = NULL;

    return (response == Gtk::RESPONSE_OK);
}

void* create_kerning_format_dlg(FVContext** p_fv_context1,
                                FVContext** p_fv_context2, int width,
                                int height) {
    // Take ownership of *p_fv_context
    std::shared_ptr<FVContext> context1(*p_fv_context1, &free);
    std::shared_ptr<FVContext> context2(*p_fv_context2, &free);
    KerningFormat* kern_fmt_dlg =
        new KerningFormat(context1, context2, width, height);
    *p_fv_context1 = NULL;
    *p_fv_context2 = NULL;

    // To prevent issues with multiple inheritance, the final static void*
    // casting should occur only to/from ICharGridContainter*
    return dynamic_cast<ICharGridContainter*>(kern_fmt_dlg);
}

bool run_kerning_format_dlg(void** kf_opaque, KFDlgData* kf_data) {
    // To prevent issues with multiple inheritance, the final static void*
    // casting should occur only to/from ICharGridContainter*
    auto kerning_format_dlg = dynamic_cast<KerningFormat*>(
        static_cast<ICharGridContainter*>(*kf_opaque));
    Gtk::ResponseType response = kerning_format_dlg->run(kf_data);

    // Destroy dialog and reset pointer
    delete kerning_format_dlg;
    *kf_opaque = NULL;

    return (response == Gtk::RESPONSE_OK);
}

void register_py_menu_item_in_gtk(const PyMenuSpec* spec, const char* gtk_accel,
                                  int flags) {
    ff::views::register_py_menu_item(spec, gtk_accel, flags);
}
