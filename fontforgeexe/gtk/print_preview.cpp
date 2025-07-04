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

#include "print_preview.hpp"

#include "intl.h"
#include "utils.hpp"
#include "win32_utils.hpp"

namespace ff::dlg {

static bool draw_preview_area(const Cairo::RefPtr<Cairo::Context>& cr) {
    // Dummy red coloring
    cr->set_source_rgb(1.0, 0.0, 0.0);
    cr->paint();

    return true;
}

PrintPreviewWidget::PrintPreviewWidget() : fixed_wrapper(0.5, 0.5, 0.5) {
    if (is_win32_display()) {
        // In Windows the GTK preview tab is embedded in the native Print
        // Dialog. The native dialog is not resizable, and its size can't be
        // queried. We must decide on a reasonable size for print preview
        // widget, and request the allocation in advance.
        Gdk::Rectangle preview_rectangle = get_win32_print_preview_size();
        set_size_request(preview_rectangle.get_width(),
                         preview_rectangle.get_height());
    }

    build_compound_preview_area();
    dummy_label = Gtk::Label("Dummy label");

    attach(fixed_wrapper, 0, 0);
    attach(dummy_label, 1, 0);
    show_all();
}

Glib::ustring PrintPreviewWidget::label() { return _("Preview"); }

void PrintPreviewWidget::draw_page_cb(
    const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr) {
    Cairo::RefPtr<Cairo::Context> cr = context->get_cairo_context();

    // White background
    cr->set_source_rgb(1, 1, 1);
    cr->paint();

    // Print sample text in black
    cr->select_font_face("Sans", Cairo::FontSlant::FONT_SLANT_NORMAL,
                         Cairo::FontWeight::FONT_WEIGHT_NORMAL);
    cr->set_font_size(24.0);
    cr->move_to(100.0, 100.0);
    cr->set_source_rgb(0, 0, 0);
    cr->show_text("Hello World");
}

void PrintPreviewWidget::build_compound_preview_area() {
    // The preview area contains a page preview with a 3D shadow on a grey
    // background, in a Firefox style. Unfortunately, 3D shadow is difficult to
    // draw manually in Cairo, so we implement it using CSS. This requires a
    // Gtk::Fixed container, on which the preview widget can be placed in a free
    // manner. To support CSS styling, the preview widget is wrapped with
    // Gtk::Box, which provides a CSS node.
    ui_utils::apply_css(box_wrapper, "box { box-shadow: 3pt 3pt 3pt black;}");

    fixed_wrapper.set_hexpand(true);
    fixed_wrapper.set_vexpand(true);
    preview_area.signal_draw().connect(&draw_preview_area);

    // Localize the page-sized preview area inside the allowed space.
    fixed_wrapper.signal_size_allocate().connect([this](Gtk::Allocation& a) {
        preview_area.set_size_request(a.get_width() / 2, a.get_height() / 2);

        // queue_resize() will not work right inside the slot. We need to defer
        // it as a separate subsequent event.
        Glib::signal_idle().connect_once([this]() {
            preview_area.queue_resize();
            queue_draw();
        });
    });

    fixed_wrapper.put(box_wrapper, 50, 50);
    box_wrapper.pack_start(preview_area, true, true);
}

}  // namespace ff::dlg
